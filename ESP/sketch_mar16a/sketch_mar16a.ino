#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>

/*===========================================================================
                        CONFIGURATION
===========================================================================*/
#define WIFI_SSID           "ZzZz"
#define WIFI_PASSWORD       "Esmail_122001"
#define SERVER_IP           "192.168.1.2"
#define SERVER_PORT         5000
#define POLL_INTERVAL_MS    30000
#define RESET_PIN           D1
#define STM32_BAUD          115200

/* Command bytes — must match COMMANDS dict in server.py and ISRs in SwM.c */
#define CMD_LED_ON          0x01
#define CMD_LED_OFF         0x02
#define CMD_BTLD_JUMP       0x03
#define CMD_TRIGGER_OTA     0x04
#define CMD_RUN_TIME        0x05
#define CMD_GET_VERSION     0xA1

/*===========================================================================
                        GLOBALS
===========================================================================*/
ESP8266WebServer cmdServer(80);   /* HTTP server for dashboard commands */

String   currentVersion = "0.0.0";
uint32_t lastPollTime   = 0;
bool     flashing       = false;
bool     otaTrigger     = false;   /* set true → force OTA on next poll */
bool     forceOTA       = false;   /* set true → bypass version match   */

/*===========================================================================
                        DEBUG  (suppressed while flashing)
===========================================================================*/
void dbg(const String& msg)
{
    if (!flashing)
        Serial.println("[ESP] " + msg);
}

/*===========================================================================
                        WIFI
===========================================================================*/
void connectWiFi()
{
    dbg("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        if (++attempts > 40)
        {
            dbg("WiFi failed! Restarting...");
            ESP.restart();
        }
    }
    dbg("WiFi OK  IP: " + WiFi.localIP().toString());
    delay(1000);   /* let the IP stack settle before any HTTP call */
}

/*===========================================================================
                        REGISTER WITH FLASK SERVER
                        Server stores our IP → can push commands to us
===========================================================================*/
bool registerWithServer()
{
    /* Retry up to 5 times — -1 (connection refused) happens when the
       network stack isn't fully ready or the server is momentarily busy. */
    for (int attempt = 1; attempt <= 5; attempt++)
    {
        WiFiClient client;
        HTTPClient http;

        String url = "http://" + String(SERVER_IP) + ":" + String(SERVER_PORT)
                     + "/api/esp_register";
        http.begin(client, url);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        http.setTimeout(3000);

        String body = "ip=" + WiFi.localIP().toString();
        int    code = http.POST(body);
        http.end();

        dbg("Register attempt " + String(attempt) + "/5 → " + String(code));

        if (code == 200)
        {
            dbg("Registered OK  (" + WiFi.localIP().toString() + ")");
            return true;
        }
        if (attempt < 5) delay(3000);
    }
    dbg("Registration failed — will retry in loop");
    return false;
}

/*===========================================================================
                        HTTP SERVER  — /cmd
                        Called by the Flask server to forward commands
===========================================================================*/
void handleCmd()
{
    if (!cmdServer.hasArg("byte"))
    {
        cmdServer.send(400, "text/plain", "missing param: byte");
        return;
    }

    uint8_t b = (uint8_t)strtol(cmdServer.arg("byte").c_str(), NULL, 16);

    if (b == CMD_TRIGGER_OTA)
    {
        /* "Trigger Update" via WiFi → force OTA poll immediately */
        otaTrigger = true;
        cmdServer.send(200, "text/plain", "ota_trigger");
        dbg("OTA trigger via HTTP");
        return;
    }

    if (b == CMD_BTLD_JUMP)
    {
        /* Forward PC3=LOW to STM32, then force OTA (which does reset+flash) */
        if (!flashing)
            Serial.write((uint8_t)CMD_BTLD_JUMP);
        forceOTA   = true;
        otaTrigger = true;
        cmdServer.send(200, "text/plain", "btld_jump+ota");
        dbg("BTLD_Jump + force OTA via HTTP");
        return;
    }

    /* All other commands → forward byte to STM32 UART1 */
    if (!flashing)
        Serial.write(b);

    cmdServer.send(200, "text/plain", "ok");
    dbg("Forwarded 0x" + String(b, HEX) + " to STM32");
}

/*===========================================================================
                        HTTP SERVER  — /health
                        Dashboard can check ESP is alive and get its version
===========================================================================*/
void handleHealth()
{
    String body = "{\"ip\":\"" + WiFi.localIP().toString()
                + "\",\"rssi\":" + String(WiFi.RSSI())
                + ",\"stm_ver\":\"" + currentVersion + "\"}";
    cmdServer.send(200, "application/json", body);
}

/*===========================================================================
                        GET STM32 VERSION  (0xA1 → UART1)
===========================================================================*/
String getSTM32Version()
{
    String   version = "";
    uint32_t start   = millis();

    while (Serial.available()) Serial.read();   /* flush RX */
    Serial.write((uint8_t)CMD_GET_VERSION);

    while (millis() - start < 1500)
    {
        if (Serial.available())
        {
            char c = Serial.read();
            if (c == '\n' || c == '\r' || c == '\0') break;
            if ((c >= '0' && c <= '9') || c == '.')
                version += c;
        }
    }
    version.trim();
    dbg("STM32 ver: [" + version + "]");
    return version;
}

/*===========================================================================
                        GET SERVER VERSION  (GET /version)
===========================================================================*/
String getServerVersion()
{
    WiFiClient client;
    HTTPClient http;
    String     version = "";

    String url = "http://" + String(SERVER_IP) + ":" + String(SERVER_PORT) + "/version";
    http.begin(client, url);

    int code = http.GET();
    if (code == HTTP_CODE_OK)
    {
        version = http.getString();
        version.trim();
        dbg("Server ver: " + version);
    }
    else
    {
        dbg("GET /version failed → " + String(code));
    }
    http.end();
    return version;
}

/*===========================================================================
                        STM32 HARDWARE RESET  (NRST pulse)
===========================================================================*/
void triggerSTM32Reset()
{
    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN, LOW);
    delay(100);
    pinMode(RESET_PIN, INPUT_PULLUP);
    delay(1500);   /* bootloader init time */
}

/*===========================================================================
                        DOWNLOAD FIRMWARE TO RAM
===========================================================================*/
uint8_t* downloadFirmware(int& totalBytes)
{
    Serial.end();   /* silence UART during heavy WiFi transfer */

    WiFiClient client;
    HTTPClient http;

    String url = "http://" + String(SERVER_IP) + ":" + String(SERVER_PORT) + "/firmware";
    http.begin(client, url);
    http.setTimeout(15000);

    int code = http.GET();
    if (code != HTTP_CODE_OK)
    {
        http.end();
        Serial.begin(STM32_BAUD); delay(200);
        return NULL;
    }

    totalBytes = http.getSize();
    if (totalBytes <= 0)
    {
        http.end();
        Serial.begin(STM32_BAUD); delay(200);
        return NULL;
    }

    uint8_t* buf = (uint8_t*)malloc(totalBytes);
    if (!buf)
    {
        http.end();
        Serial.begin(STM32_BAUD); delay(200);
        return NULL;
    }

    WiFiClient* stream    = http.getStreamPtr();
    int         totalRead = 0;
    uint32_t    deadline  = millis() + 15000;

    while (totalRead < totalBytes && millis() < deadline)
    {
        int avail = stream->available();
        if (avail > 0)
        {
            int chunk  = min(avail, 128);
            totalRead += stream->readBytes(buf + totalRead, chunk);
            deadline   = millis() + 5000;
        }
        yield();
    }

    http.end();
    Serial.begin(STM32_BAUD); delay(500);

    if (totalRead != totalBytes)
    {
        free(buf);
        dbg("Download incomplete: " + String(totalRead) + "/" + String(totalBytes));
        return NULL;
    }
    dbg("Downloaded " + String(totalBytes) + " bytes");
    return buf;
}

/*===========================================================================
                        FLASH STM32
                        flashing == true: NO Serial.println!
===========================================================================*/
bool flashSTM32(uint8_t* buf, int totalBytes)
{
    triggerSTM32Reset();
    Serial.flush();
    delay(2000);
    Serial.write((uint8_t)0x55);          /* bootloader sync byte */
    Serial.write(buf, totalBytes);
    Serial.flush();

    delay(2000);   /* BTLD writes flash + soft resets */
    return true;
}

/*===========================================================================
                        NOTIFY SERVER  (POST /status)
===========================================================================*/
void notifyServer(const String& version, bool success)
{
    WiFiClient client;
    HTTPClient http;

    String url = "http://" + String(SERVER_IP) + ":" + String(SERVER_PORT) + "/status";
    http.begin(client, url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String body = "version=" + version + "&status=" + (success ? "ok" : "failed");
    int    code = http.POST(body);
    dbg("POST /status → " + String(code));
    http.end();
}

/*===========================================================================
                        FULL OTA FLOW
===========================================================================*/
void checkAndFlash(bool force)
{
    /* Step 1: STM32 current version */
    currentVersion = getSTM32Version();
    if (currentVersion == "")
    {
        dbg("STM32 not responding — skip OTA");
        return;
    }

    /* Step 2: server version */
    String serverVersion = getServerVersion();
    if (serverVersion == "")
    {
        dbg("Server unreachable — skip OTA");
        return;
    }

    if (!force && serverVersion == currentVersion)
    {
        dbg("Up to date: v" + currentVersion);
        return;
    }

    dbg("Flashing: " + currentVersion + " → " + serverVersion);

    /* Step 3: download */
    int      totalBytes = 0;
    uint8_t* buf        = downloadFirmware(totalBytes);
    if (!buf)
    {
        dbg("Download failed!");
        notifyServer(serverVersion, false);
        return;
    }

    /* Step 4: flash */
    flashing     = true;
    bool success = flashSTM32(buf, totalBytes);
    free(buf);
    flashing     = false;

    /* Step 5: verify */
    delay(1000);
    currentVersion = getSTM32Version();
    bool verified  = (currentVersion == serverVersion);
    dbg("Post-flash ver: " + currentVersion + (verified ? " ✓" : " ✗ MISMATCH"));

    /* Step 6: report */
    notifyServer(serverVersion, verified);
}

/*===========================================================================
                        SETUP
===========================================================================*/
void setup()
{
    pinMode(RESET_PIN, INPUT_PULLUP);   /* keep NRST HIGH during ESP boot */
    delay(2000);                        /* let ESP boot garbage settle     */

    Serial.begin(STM32_BAUD);
    dbg("ESP OTA + Control node starting...");

    connectWiFi();
    registerWithServer();

    /* Start HTTP command server */
    cmdServer.on("/cmd",    HTTP_POST, handleCmd);
    cmdServer.on("/health", HTTP_GET,  handleHealth);
    cmdServer.begin();
    dbg("CMD server listening on port 80");
    dbg("Polling every " + String(POLL_INTERVAL_MS / 1000) + "s");
}

/*===========================================================================
                        LOOP
===========================================================================*/
void loop()
{
    /* WiFi watchdog */
    if (WiFi.status() != WL_CONNECTED)
    {
        dbg("WiFi lost — reconnecting...");
        connectWiFi();
        registerWithServer();
    }

    /* Serve incoming HTTP commands from dashboard */
    cmdServer.handleClient();

    /* Re-register with server every 2 min so it keeps our IP
       (handles server restarts and first-boot timing failures) */
    static uint32_t lastRegTime = 0;
    if (!flashing && millis() - lastRegTime >= 120000UL)
    {
        lastRegTime = millis();
        registerWithServer();
    }

    /* Check for OTA trigger byte from STM32 on UART1 */
    while (!flashing && Serial.available())
    {
        uint8_t b = (uint8_t)Serial.read();
        if (b == CMD_TRIGGER_OTA)
        {
            dbg("OTA trigger byte from STM32");
            otaTrigger = true;
        }
    }

    /* Periodic OTA poll (or immediate if triggered) */
    bool doNow = otaTrigger || (millis() - lastPollTime >= POLL_INTERVAL_MS);
    if (doNow)
    {
        bool force  = forceOTA;
        otaTrigger  = false;
        forceOTA    = false;
        lastPollTime = millis();

        dbg("--- OTA check (force=" + String(force) + ") ---");
        checkAndFlash(force);
    }
}
