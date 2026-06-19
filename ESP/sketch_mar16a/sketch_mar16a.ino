#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <time.h>

/*===========================================================================
                        CONFIGURATION
===========================================================================*/
#define WIFI_SSID        "ZzZz"
#define WIFI_PASSWORD    "Esmail_122001"
#define SERVER_IP        "192.168.1.19"
#define SERVER_PORT      5000
#define TELEMETRY_PORT   5001          /* UDP — fire-and-forget, no handshake */
#define POLL_INTERVAL_MS 30000
#define RESET_PIN        D1
#define STM32_BAUD       115200

/* NTP — change offset to match your timezone (seconds)
 * UTC+0=0  UTC+1=3600  UTC+2=7200  UTC+3=10800 */
#define NTP_SERVER         "pool.ntp.org"
#define NTP_TIMEZONE_OFFSET 7200        /* UTC+2 (Egypt) */
#define NTP_SYNC_INTERVAL_MS 3600000UL  /* re-sync every 1 hour */

#define CMD_LED_ON      0x01
#define CMD_LED_OFF     0x02
#define CMD_BTLD_JUMP   0x03
#define CMD_TRIGGER_OTA 0x04
#define CMD_RUN_TIME    0x05
#define CMD_GET_VERSION 0xA1

/*===========================================================================
                        GLOBALS
===========================================================================*/
ESP8266WebServer cmdServer(80);
WiFiUDP          udp;

String   currentVersion = "";
uint32_t lastPollTime   = 0;
uint32_t lastRegTime    = 0;
bool     flashing       = false;
bool     otaTrigger     = false;
bool     forceOTA       = false;

/* Serial line accumulator — non-blocking */
String   rxBuf = "";

/* NTP */
uint32_t lastNtpSync = 0;
bool     ntpSynced   = false;

/*===========================================================================
                        UDP TELEMETRY  — ~0 ms, no TCP handshake
===========================================================================*/
void sendTelemetry(const String& line)
{
    udp.beginPacket(SERVER_IP, TELEMETRY_PORT);
    udp.write((const uint8_t*)line.c_str(), line.length());
    udp.endPacket();
}

/*===========================================================================
                        HTTP HELPERS
===========================================================================*/
static String serverUrl(const char* path)
{
    return "http://" SERVER_IP ":" + String(SERVER_PORT) + path;
}

static int httpPost(const String& url, const String& body,
                    const char* ct = "application/x-www-form-urlencoded",
                    int timeout = 2000)
{
    WiFiClient  client;
    HTTPClient  http;
    http.begin(client, url);
    http.addHeader("Content-Type", ct);
    http.setTimeout(timeout);
    int code = http.POST(body);
    http.end();
    return code;
}

/*===========================================================================
                        NTP TIME SYNC
===========================================================================*/

/* Sync ESP internal clock from NTP server.
 * Waits up to 10 seconds for a valid timestamp. */
void syncNTP()
{
    configTime(NTP_TIMEZONE_OFFSET, 0, NTP_SERVER, "time.nist.gov");

    uint32_t deadline = millis() + 10000;
    while (time(nullptr) < 1000000000UL && millis() < deadline)
    {
        delay(100);
        yield();
    }

    if (time(nullptr) > 1000000000UL)
    {
        ntpSynced    = true;
        lastNtpSync  = millis();
        sendTelemetry("[NTP] synced: " + String(time(nullptr)));
    }
    else
    {
        sendTelemetry("[NTP] sync failed — will retry");
    }
}

/* Send current time to STM32 over UART1 as "TIME:HH:MM:SS\n"
 * STM32 UART1_ISR parses this and calls RTC_SetTime(). */
void sendTimeToSTM32()
{
    if (!ntpSynced) return;

    time_t    now = time(nullptr);
    struct tm*  t = localtime(&now);

    char buf[20];
    snprintf(buf, sizeof(buf), "TIME:%02d:%02d:%02d",
             t->tm_hour, t->tm_min, t->tm_sec);

    if (!flashing)
    {
        Serial.print(buf);
        Serial.print('\r');   /* use \r only — \n (0x0A) = CMD_STANDBY on STM32 */
        sendTelemetry(String("[NTP] sent to STM32: ") + buf);
    }
}

/*===========================================================================
                        WIFI
===========================================================================*/
void connectWiFi()
{
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    uint32_t t = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - t > 15000) { ESP.restart(); }
        delay(250);
        yield();
    }
}

/*===========================================================================
                        REGISTER WITH SERVER
===========================================================================*/
void registerWithServer()
{
    String body = "ip=" + WiFi.localIP().toString();
    for (int i = 0; i < 3; i++)
    {
        if (httpPost(serverUrl("/api/esp_register"), body) == 200) return;
        if (i < 2) delay(1500);
    }
}

/*===========================================================================
                        HTTP /cmd  — commands from dashboard
===========================================================================*/
void handleCmd()
{
    if (!cmdServer.hasArg("byte"))
    {
        cmdServer.send(400, "text/plain", "missing byte");
        return;
    }
    uint8_t b = (uint8_t)strtol(cmdServer.arg("byte").c_str(), NULL, 16);

    if (b == CMD_TRIGGER_OTA)
    {
        otaTrigger = true;
        cmdServer.send(200, "text/plain", "ota_trigger");
        return;
    }
    if (b == CMD_BTLD_JUMP)
    {
        if (!flashing) Serial.write((uint8_t)CMD_BTLD_JUMP);
        forceOTA = otaTrigger = true;
        cmdServer.send(200, "text/plain", "btld_jump+ota");
        return;
    }

    if (!flashing) Serial.write(b);
    cmdServer.send(200, "text/plain", "ok");
}

void handleHealth()
{
    String body = "{\"ip\":\"" + WiFi.localIP().toString()
                + "\",\"rssi\":"        + String(WiFi.RSSI())
                + ",\"stm_ver\":\""     + currentVersion + "\"}";
    cmdServer.send(200, "application/json", body);
}

/*===========================================================================
                        GET STM32 VERSION
        Sends 0xA1, waits for "VER:x.x.x\n", forwards all other lines
        as telemetry so BTLD/APP output appears in the dashboard terminal.
===========================================================================*/
String getSTM32Version()
{
    while (Serial.available()) Serial.read();   /* flush stale bytes */
    Serial.write((uint8_t)CMD_GET_VERSION);

    String   line = "";
    uint32_t end  = millis() + 3000;

    while (millis() < end)
    {
        while (Serial.available())
        {
            char c = (char)Serial.read();
            if (c == '\n')
            {
                line.trim();
                if (line.startsWith("VER:"))
                {
                    String ver = line.substring(4);
                    ver.trim();
                    sendTelemetry(line);   /* update dashboard version pill */
                    return ver;
                }
                if (line.length() > 0) sendTelemetry(line);
                line = "";
            }
            else if (c != '\r' && line.length() < 128)
            {
                line += c;
            }
        }
        yield();
    }
    return "";   /* timeout — STM not ready */
}

/*===========================================================================
                        OTA FLOW
===========================================================================*/
void triggerSTM32Reset()
{
    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN, LOW);
    delay(50);
    pinMode(RESET_PIN, INPUT_PULLUP);
    delay(800);   /* bootloader needs ~500 ms to init */
}

String getServerVersion()
{
    WiFiClient client;
    HTTPClient http;
    http.begin(client, serverUrl("/version"));
    http.setTimeout(3000);
    String ver = "";
    if (http.GET() == HTTP_CODE_OK) { ver = http.getString(); ver.trim(); }
    http.end();
    return ver;
}

uint8_t* downloadFirmware(int& outSize)
{
    Serial.end();   /* pause UART during heavy WiFi transfer */

    WiFiClient client;
    HTTPClient http;
    http.begin(client, serverUrl("/firmware"));
    http.setTimeout(15000);

    int code = http.GET();
    if (code != HTTP_CODE_OK)
    {
        http.end();
        Serial.begin(STM32_BAUD);
        return NULL;
    }

    outSize = http.getSize();
    if (outSize <= 0) { http.end(); Serial.begin(STM32_BAUD); return NULL; }

    uint8_t* buf = (uint8_t*)malloc(outSize);
    if (!buf)      { http.end(); Serial.begin(STM32_BAUD); return NULL; }

    WiFiClient* stream   = http.getStreamPtr();
    int         received = 0;
    uint32_t    deadline = millis() + 20000;

    while (received < outSize && millis() < deadline)
    {
        int avail = stream->available();
        if (avail > 0)
        {
            int n     = min(avail, 256);
            received += stream->readBytes(buf + received, n);
        }
        yield();
    }

    http.end();
    Serial.begin(STM32_BAUD);
    delay(200);

    if (received != outSize) { free(buf); return NULL; }
    return buf;
}

bool flashSTM32(uint8_t* buf, int len)
{
    triggerSTM32Reset();
    Serial.write((uint8_t)0x55);   /* bootloader sync byte */
    Serial.write(buf, len);
    Serial.flush();
    delay(1500);   /* BTLD: CRC verify + remaining flash writes + reset chain */
    return true;
}

void notifyServer(const String& ver, bool ok)
{
    String body = "version=" + ver + "&status=" + (ok ? "ok" : "failed");
    httpPost(serverUrl("/status"), body);
}

void checkAndFlash(bool force)
{
    currentVersion = getSTM32Version();
    if (currentVersion.isEmpty()) return;

    String serverVer = getServerVersion();
    if (serverVer.isEmpty()) return;

    if (!force && serverVer == currentVersion) return;

    int      totalBytes = 0;
    uint8_t* buf        = downloadFirmware(totalBytes);
    if (!buf) { notifyServer(serverVer, false); return; }

    flashing = true;
    flashSTM32(buf, totalBytes);
    free(buf);
    flashing = false;

    /* Wait for reset chain: BTLD → BM → APP → RTOS ready */
    delay(3000);
    currentVersion = getSTM32Version();
    bool ok = (currentVersion == serverVer);
    notifyServer(serverVer, ok);
}

/*===========================================================================
                        SETUP
===========================================================================*/
void setup()
{
    pinMode(RESET_PIN, INPUT_PULLUP);
    Serial.begin(STM32_BAUD);
    delay(300);

    connectWiFi();
    udp.begin(TELEMETRY_PORT);
    syncNTP();          /* get real time from internet */
    registerWithServer();
    sendTimeToSTM32();  /* set STM32 RTC on boot      */

    cmdServer.on("/cmd",    HTTP_POST, handleCmd);
    cmdServer.on("/health", HTTP_GET,  handleHealth);
    cmdServer.begin();
}

/*===========================================================================
                        LOOP
===========================================================================*/
void loop()
{
    /* WiFi watchdog */
    if (WiFi.status() != WL_CONNECTED)
    {
        connectWiFi();
        registerWithServer();
    }

    /* Always serve HTTP commands first — keep latency low */
    cmdServer.handleClient();

    /* Re-register every 2 min */
    if (millis() - lastRegTime >= 120000UL)
    {
        lastRegTime = millis();
        registerWithServer();
    }

    /* Non-blocking serial read — assemble complete lines, forward via UDP */
    if (!flashing)
    {
        while (Serial.available())
        {
            uint8_t b = Serial.read();

            if (b == CMD_TRIGGER_OTA)
            {
                otaTrigger = true;
                rxBuf      = "";
                continue;
            }
            if (b == '\n')
            {
                rxBuf.trim();
                if (rxBuf.length() > 0) sendTelemetry(rxBuf);
                rxBuf = "";
            }
            else if (b != '\r' && (b >= 0x20 || b == '\t') && rxBuf.length() < 256)
            {
                rxBuf += (char)b;
            }
        }
    }

    /* NTP re-sync every hour and resend to STM32 */
    if (millis() - lastNtpSync >= NTP_SYNC_INTERVAL_MS)
    {
        syncNTP();
        sendTimeToSTM32();
    }

    /* OTA check — only when triggered or poll interval elapsed */
    if (!flashing && (otaTrigger || millis() - lastPollTime >= POLL_INTERVAL_MS))
    {
        bool force  = forceOTA;
        otaTrigger  = false;
        forceOTA    = false;
        lastPollTime = millis();
        checkAndFlash(force);
    }
}
