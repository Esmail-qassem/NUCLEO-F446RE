#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

/*===========================================================================
                        CONFIGURATION
===========================================================================*/
#define WIFI_SSID        "ZzZz"
#define WIFI_PASSWORD    "Esmail_122001"
#define SERVER_IP        "192.168.1.18"
#define SERVER_PORT      5000
#define POLL_INTERVAL_MS 30000
#define RESET_PIN        D1
#define STM32_BAUD       115200
#define CMD_GET_VERSION  0xA1

/*===========================================================================
                        GLOBALS
===========================================================================*/
String   currentVersion = "0.0.0";
uint32_t lastPollTime   = 0;
bool     flashing       = false;

/*===========================================================================
                        DEBUG
                        Only prints when not flashing — safe for STM32 UART
===========================================================================*/
void dbg(String msg)
{
    if (!flashing)
    {
        Serial.println("[ESP] " + msg);
    }
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
        attempts++;
        if (attempts > 40)
        {
            dbg("WiFi failed! Restarting...");
            ESP.restart();
        }
    }

    dbg("WiFi connected! IP: " + WiFi.localIP().toString());
}

/*===========================================================================
                        GET STM32 VERSION
                        Sends 0xA1 command → STM32 replies with version string
===========================================================================*/
String getSTM32Version()
{
    String   version = "";
    uint32_t start   = millis();

    /* Send version request command */
    Serial.write(CMD_GET_VERSION);

    /* Wait up to 1 second for response */
    while (millis() - start < 1000)
    {
        if (Serial.available())
        {
            char c = Serial.read();
            if (c == '\0') break;  // null terminator = end of string
            version += c;
        }
    }

    return version;
}

/*===========================================================================
                        STM32 RESET
                        ⚠️ Called only when flashing == true
                        ⚠️ NO Serial.println here!
===========================================================================*/
void triggerSTM32Reset()
{
    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN, LOW);
    delay(100);               // hold NRST LOW for 100ms
    pinMode(RESET_PIN, INPUT_PULLUP);
    delay(1000);              // wait 2s for BTLD to initialize
}

/*===========================================================================
                        CHECK SERVER VERSION
===========================================================================*/
String getServerVersion()
{
    WiFiClient  client;
    HTTPClient  http;
    String      version = "";

    String url = "http://" + String(SERVER_IP) + ":" + String(SERVER_PORT) + "/version";
    http.begin(client, url);

    int httpCode = http.GET();
    dbg("GET /version -> " + String(httpCode));

    if (httpCode == HTTP_CODE_OK)
    {
        version = http.getString();
        version.trim();
        dbg("Server: " + version + " | STM32: " + currentVersion);
    }
    else
    {
        dbg("Version check failed!");
    }

    http.end();
    return version;
}

/*===========================================================================
                        DOWNLOAD FIRMWARE TO RAM
                        Downloads entire .bin before touching STM32
===========================================================================*/
uint8_t* downloadFirmware(int &totalBytes)
{
    /* Stop UART TX completely during WiFi download */
    Serial.end();

    WiFiClient  client;
    HTTPClient  http;

    String url = "http://" + String(SERVER_IP) + ":" + String(SERVER_PORT) + "/firmware";

    http.begin(client, url);
    http.setTimeout(10000);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK)
    {
        Serial.begin(STM32_BAUD);
        delay(100);
        http.end();
        return NULL;
    }

    totalBytes = http.getSize();

    uint8_t* buf = (uint8_t*)malloc(totalBytes);
    if (buf == NULL)
    {
        Serial.begin(STM32_BAUD);
        delay(100);
        http.end();
        return NULL;
    }

    WiFiClient* stream    = http.getStreamPtr();
    int         totalRead = 0;
    uint32_t    startTime = millis();

    while (totalRead < totalBytes)
    {
        if (millis() - startTime > 10000)
        {
            free(buf);
            Serial.begin(STM32_BAUD);
            delay(100);
            http.end();
            return NULL;
        }

        int available = stream->available();
        if (available > 0)
        {
            int toRead  = min(available, 64);
            totalRead  += stream->readBytes(buf + totalRead, toRead);
            startTime   = millis();
        }
        yield();
    }

    http.end();

    if (totalRead != totalBytes)
    {
        free(buf);
        Serial.begin(STM32_BAUD);
        delay(100);
        return NULL;
    }

    /* Re-enable Serial after download */
    Serial.begin(STM32_BAUD);
    delay(500);  // let UART settle

    return buf;
}
/*===========================================================================
                        FLASH STM32
                        ⚠️ flashing == true here — NO Serial.println allowed!
                        ⚠️ Serial.write() ONLY for raw firmware bytes!
===========================================================================*/
bool flashSTM32(uint8_t* buf, int totalBytes)
{
    /* Reset STM32 -> enters BTLD, wait 2s for init */
    triggerSTM32Reset();

    /* Flush any pending bytes before sending */
    Serial.flush();

    /* Send ALL firmware bytes at once */
    Serial.write(0x55);
    Serial.write(buf, totalBytes);

    /* Wait for BTLD to receive, verify and soft reset
     * BTLD timeout: 1s flush + 2s verify = 3s after last byte */
    delay(1000);

    return true;
}

/*===========================================================================
                        NOTIFY SERVER
===========================================================================*/
void notifyServer(String version, bool success)
{
    WiFiClient  client;
    HTTPClient  http;

    String url = "http://" + String(SERVER_IP) + ":" + String(SERVER_PORT) + "/status";
    http.begin(client, url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String body     = "version=" + version + "&status=" + (success ? "ok" : "failed");
    int    httpCode = http.POST(body);

    dbg("Notify -> " + String(httpCode));
    http.end();
}

/*===========================================================================
                        SETUP
===========================================================================*/
void setup()
{
    /* Keep NRST HIGH during ESP boot
     * Prevents ESP boot garbage from triggering STM32 reset */
    pinMode(RESET_PIN, INPUT_PULLUP);

    /* Wait for ESP boot garbage to finish BEFORE starting Serial */
    delay(2000);

    /* Start Serial — used for STM32 communication + debug */
    Serial.begin(STM32_BAUD);

    dbg("ESP OTA Client starting...");
    connectWiFi();
    dbg("Ready! Polling every 30s...");
}

/*===========================================================================
                        LOOP
===========================================================================*/
void loop()
{
    /* Reconnect WiFi if lost */
    if (WiFi.status() != WL_CONNECTED)
    {
        dbg("WiFi lost! Reconnecting...");
        connectWiFi();
    }

    if (millis() - lastPollTime >= POLL_INTERVAL_MS)
    {
        lastPollTime = millis();

        /*--- Step 1: Get actual version from STM32 ---*/
        currentVersion = getSTM32Version();

        if (currentVersion == "")
        {
            dbg("STM32 not responding — skipping poll");
            return;
        }

        dbg("STM32 version: " + currentVersion);
        dbg("--- Polling server ---");

        /*--- Step 2: Get latest version from server ---*/
        String serverVersion = getServerVersion();

        if (serverVersion == "")
        {
            dbg("Server unreachable — skipping poll");
            return;
        }

        if (serverVersion != currentVersion)
        {
            dbg("New firmware: v" + serverVersion);

            /*--- Step 3: Download firmware to RAM ---*/
            int      totalBytes     = 0;
            uint8_t* firmwareBuffer = downloadFirmware(totalBytes);

            if (firmwareBuffer == NULL)
            {
                dbg("Download failed!");
                notifyServer(serverVersion, false);
                return;
            }

            /*--- Step 4: Flash STM32 (Serial goes silent) ---*/
            flashing     = true;
            bool success = flashSTM32(firmwareBuffer, totalBytes);
            free(firmwareBuffer);
            flashing     = false;

            /*--- Step 5: Confirm version from STM32 ---*/
            delay(1000);  // wait for APP to boot
            currentVersion = getSTM32Version();
            dbg("Confirmed STM32 version: " + currentVersion);

            /*--- Step 6: Notify server ---*/
            if (success)
            {
                notifyServer(serverVersion, true);
                dbg("Update complete! v" + currentVersion);
            }
            else
            {
                notifyServer(serverVersion, false);
                dbg("Update FAILED!");
            }
        }
        else
        {
            dbg("Up to date: v" + currentVersion);
        }
    }
}
