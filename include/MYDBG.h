// Datei: MYDBG.h – erste grüne webseite 04.2025
#ifndef MYDBG_H
#define MYDBG_H

#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h> // neu
#include <time.h>
#include <esp_task_wdt.h>
#include <ArduinoJson.h>

#define MYDBG_MAX_LOGFILES 3
#define MYDBG_WDT_DEFAULT 10
#define MYDBG_WDT_EXTENDED 300
// Automatische Initialisierung aktivieren/deaktivieren
// Wenn gesetzt, wird KEINE automatische Initialisierung gemacht
// #define MYDBG_NO_AUTOINIT

inline bool MYDBG_timeInitDone = false;
static bool MYDBG_warnedAboutTime = false;
inline bool MYDBG_isEnabled = true;
inline bool MYDBG_stopEnabled = true;
inline bool MYDBG_webDebugEnabled = false;
inline bool MYDBG_webClientActive = false;
inline bool MYDBG_filesystemReady = false;
inline bool MYDBG_menuFirstCall = true;
inline unsigned long MYDBG_menuTimeout = 5000;

AsyncWebServer MYDBG_server(80); // Webserver auf Port 80
AsyncWebSocket MYDBG_ws("/ws");  // WebSocket auf Pfad /ws

inline void MYDBG_streamWebLine(const String &msg);
inline void MYDBG_streamWebLineJSON(const String &msg, const String &varName, const String &varValue, const String &func, int zeile);
inline String MYDBG_getTimestamp();
inline void MYDBG_startWebDebug();
inline void MYDBG_setWatchdog(int sekunden);
inline void MYDBG_stopAusgabe(const String &msg, const String &varName, const String &varValue, const String &func, int zeile);
inline void MYDBG_logToJson(const String &text, const String &func, int line, const String &varName, const String &varValue);
inline void MYDBG_writeStatusFile(const String &msg, const String &func, int line, const String &varName, const String &varValue);
inline void MYDBG_initFilesystem();
inline void MYDBG_MENUE();
inline void MYDBG_initTime(const char *ntpServer = "pool.ntp.org");
void deleteJsonLogs();

inline void MYDBG_autoInit()
{
#ifndef MYDBG_NO_AUTOINIT
    static bool alreadyInitialized = false;

    if (!MYDBG_filesystemReady)
        MYDBG_initFilesystem(); // Gibt nur bei Fehlern aus

    if (!MYDBG_timeInitDone)
        MYDBG_initTime(); // Gibt bereits bei Fehler selbstständig Warnung aus

    if (!MYDBG_webDebugEnabled)
    {
        MYDBG_startWebDebug(); // Kein extra Log nötig
        MYDBG_webDebugEnabled = true;
    }

    alreadyInitialized = true; // stiller Erfolg
#endif
}

#define MYDBG(...) MYDBG_WRAPPER(__VA_ARGS__, MYDBG3, MYDBG2)(__VA_ARGS__)
#define MYDBG_WRAPPER(_1, _2, _3, NAME, ...) NAME
#define MYDBG2(waitIndex, msgText) MYDBG_INTERNAL(waitIndex, msgText, "---", "---")
#define MYDBG3(waitIndex, msgText, var) MYDBG_INTERNAL(waitIndex, msgText, #var, String(var))

#define MYDBG_INTERNAL(waitIndex, msgText, varName, varValue)                                                       \
    do                                                                                                              \
    {                                                                                                               \
        MYDBG_autoInit();                                                                                           \
        if (MYDBG_isEnabled && (!MYDBG_stopEnabled || waitIndex == 0))                                              \
        {                                                                                                           \
            Serial.println(String("[MYDBG] > ") + __LINE__ + " | " + msgText + " | " + varName + " = " + varValue); \
        }                                                                                                           \
        if (waitIndex > 0 && MYDBG_stopEnabled)                                                                     \
        {                                                                                                           \
            int ms = constrain(waitIndex * 1000, 0, 9000);                                                          \
            MYDBG_stopAusgabe(msgText, varName, varValue, __FUNCTION__, __LINE__);                                  \
            if (!MYDBG_filesystemReady)                                                                             \
                MYDBG_initFilesystem();                                                                             \
            MYDBG_logToJson(msgText, __FUNCTION__, __LINE__, varName, varValue);                                    \
            MYDBG_writeStatusFile(msgText, __FUNCTION__, __LINE__, varName, varValue);                              \
            MYDBG_streamWebLineJSON(msgText, varName, varValue, __FUNCTION__, __LINE__);                            \
            unsigned long __t0 = millis();                                                                          \
            while (millis() - __t0 < ms)                                                                            \
            {                                                                                                       \
                                                                                                                    \
                delay(10);                                                                                          \
            }                                                                                                       \
        }                                                                                                           \
    } while (0)

// Einfache Textzeile an WebClient senden
inline void MYDBG_streamWebLineJSON(const String &msg, const String &varName, const String &varValue, const String &func, int zeile)
{
    StaticJsonDocument<256> doc;
    doc["timestamp"] = MYDBG_getTimestamp();
    doc["pgmFunc"] = func;
    doc["pgmZeile"] = zeile;
    doc["msg"] = msg;
    doc["varName"] = varName;
    doc["varValue"] = varValue;
    doc["millis"] = millis();                              // NEU
    doc["resetReason"] = (int)esp_reset_reason();          // NEU
    doc["watchdog"] = (esp_reset_reason() == ESP_RST_WDT); // NEU
    // Neu: Filesystem-Auslastung ergänzen
    if (MYDBG_filesystemReady)
    {
        size_t total = LittleFS.totalBytes();
        size_t used = LittleFS.usedBytes();
        float freiProzent = 100.0 - (used * 100.0) / total;
        doc["fs_free_kb"] = (total - used) / 1024;
        doc["fs_free_percent"] = freiProzent;
    }
    else
    {
        doc["fs_free_kb"] = -1;
        doc["fs_free_percent"] = -1;
    }
    String jsonStr;
    serializeJson(doc, jsonStr);
    MYDBG_ws.textAll(jsonStr);
}

// LittleFS initialisieren
inline void MYDBG_initFilesystem()
{
    if (!LittleFS.begin(true))
    {
        Serial.println("[MYDBG] ⚠️ Fehler beim Mounten von LittleFS!");
    }
    MYDBG_filesystemReady = true;
}

// Watchdog aktivieren
inline void MYDBG_setWatchdog(int sekunden)
{
    esp_task_wdt_init(sekunden, true);
    esp_task_wdt_add(NULL);
}

// STOP-Ausgabe über Serial und Web
inline void MYDBG_stopAusgabe(const String &msg, const String &varName, const String &varValue, const String &func, int zeile)
{
    if (!MYDBG_isEnabled)
        return;
    String ausgabe = "[MYDBG] > " + String(zeile) + " | " + func + "() | " + MYDBG_getTimestamp() + " | " + millis() + " | " + msg + " | " + varName + " = " + varValue;
    Serial.println(ausgabe);
    MYDBG_streamWebLine(ausgabe);
}
inline void MYDBG_streamWebLine(const String &msg)
{
    if (MYDBG_webClientActive)
    {
        MYDBG_ws.textAll(msg);
    }
}

// Log-Eintrag in JSON-Datei schreiben
inline void MYDBG_logToJson(const String &text, const String &func, int line, const String &varName, const String &varValue)

{
    bool isWatchdogReset = (esp_reset_reason() == ESP_RST_WDT);

    StaticJsonDocument<1024> doc;
    File file = LittleFS.open("/mydbg_data.json", "r");
    if (file)
    {
        DeserializationError error = deserializeJson(doc, file);
        file.close();
        if (error)
            doc.clear();
    }

    JsonArray arr;
    if (!doc["log"].is<JsonArray>())
        arr = doc.createNestedArray("log");
    else
        arr = doc["log"].as<JsonArray>();

    while (arr.size() >= MYDBG_MAX_LOGFILES)
        arr.remove(0);

    JsonObject entry = arr.createNestedObject();
    entry["timestamp"] = MYDBG_getTimestamp();
    entry["millis"] = millis();
    entry["pgmFunc"] = func;
    entry["pgmZeile"] = line;
    entry["msg"] = text;
    entry["varName"] = varName;
    entry["varValue"] = varValue;
    entry["resetReason"] = (int)esp_reset_reason();
    if (isWatchdogReset)
        entry["watchdog"] = true;

    file = LittleFS.open("/mydbg_data.json", "w");
    if (file)
    {
        serializeJson(doc, file);
        file.close();
    }

    if (isWatchdogReset)
    {
        StaticJsonDocument<512> wdDoc;
        File wdFile = LittleFS.open("/mydbg_watchdog.json", "r");
        if (wdFile)
        {
            deserializeJson(wdDoc, wdFile);
            wdFile.close();
        }
        JsonArray wdArr;
        if (!wdDoc["watchdogs"].is<JsonArray>())
            wdArr = wdDoc.createNestedArray("watchdogs");
        else
            wdArr = wdDoc["watchdogs"].as<JsonArray>();

        JsonObject w = wdArr.createNestedObject();
        w["timestamp"] = MYDBG_getTimestamp();
        w["msg"] = text;
        w["func"] = func;
        w["line"] = line;
        w["var"] = varName;
        w["val"] = varValue;
        w["reason"] = (int)esp_reset_reason();

        File out = LittleFS.open("/mydbg_watchdog.json", "w");
        if (out)
        {
            serializeJson(wdDoc, out);
            out.close();
        }
    }
}

// Status-Log schreiben
inline void MYDBG_writeStatusFile(const String &msg, const String &func, int line, const String &varName, const String &varValue)
{
    if (!MYDBG_filesystemReady)
        MYDBG_initFilesystem();
    StaticJsonDocument<512> doc;
    doc["timestamp"] = MYDBG_getTimestamp();
    doc["millis"] = millis();
    doc["pgmFunc"] = func;
    doc["pgmZeile"] = line;
    doc["msg"] = msg;
    doc["varName"] = varName;
    doc["varValue"] = varValue;

    File file = LittleFS.open("/mydbg_status.json", "w");
    if (file)
    {
        serializeJson(doc, file);
        file.close();
    }
}

// Web-Debug-Seite starten
inline void MYDBG_startWebDebug()
{
    /*
    MYDBG_server.on("/delete_logs", HTTP_GET, [](AsyncWebServerRequest *request)
                    {
    deleteJsonLogs(); // <- deine existierende Funktion zum Löschen benutzen
    request->send(200, "text/plain", "Logdateien gelöscht.");
    Serial.println("[MYDBG] Logdateien per Web-Button gelöscht."); });
    */
    // HTTP-Handler für status.html
    MYDBG_server.on("/status.html", HTTP_GET, [](AsyncWebServerRequest *request)
                    { request->send(200, "text/html", R"rawliteral(
            
           <!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <title>MYDBG Web-Debug</title>
    <style>
        body { font-family: monospace; background: #111; color: #0f0; margin: 0; padding: 0; }
        #header { position: sticky; top: 0; background: #111; padding: 10px; z-index: 10; }
        h1 { margin: 0; font-size: 24px; }
        #controls { margin-top: 10px; }
        button { background: #0f0; color: #111; border: none; padding: 5px 10px; margin-right: 10px; cursor: pointer; }
        button:hover { background: #5f5; }
        table { width: 100%; border-collapse: collapse; table-layout: fixed; word-wrap: break-word; }
        th, td { border: 1px solid #0f0; padding: 5px; text-align: left; }
        th { background: #222; position: sticky; top: 100px; z-index: 5; }
        tr:nth-child(even) { background: #000; }
        #status { margin: 10px; }
    </style>
</head>
<body>
    <div id="header">
        <h1 id="mainTitle">MYDBG WEB-Debug</h1>
        <div id="controls">
            <button id="toggleProtocolBtn">Protokoll AUS</button>
            <button id="showJsonBtn">Logdatei anzeigen</button>
            <button id="showWatchdogBtn">Watchdog-Logs anzeigen</button>
            <button id="deleteLogsBtn">Logdateien löschen</button>
        </div>
        <div id="status">Verbindung wird aufgebaut...</div>
    </div>

    <table id="logTable">
        <thead>
            <tr>
                <th>Zeile</th>
                <th>Funktion</th>
                <th>Datum</th>
                <th>Millis</th>
                <th>Nachricht</th>
                <th>Variable</th>
                <th>Wert</th>
                <th>Watchdog</th>
                <th>ResetGrund</th>
            </tr>
        </thead>
        <tbody id="logBody"></tbody>
    </table>

    <script>
        let statusDiv = document.getElementById('status');
        let fsInfoDiv = document.createElement('div');
        fsInfoDiv.style.marginTop = "5px";
        document.getElementById('header').appendChild(fsInfoDiv);
        let logBody = document.getElementById('logBody');
        let mainTitle = document.getElementById('mainTitle');
        let toggleBtn = document.getElementById('toggleProtocolBtn');
        let protocolActive = true; // Start: Protokoll EIN
        if (location.protocol === 'https:') {
            location.href = 'http://' + location.host + location.pathname;
        }
        let conn = new WebSocket('ws://' + location.host + '/ws');
        conn.onopen = () => statusDiv.innerText = "Verbindung aktiv. Speicher unbekannt.";
        conn.onmessage = function(event) {
            if (!protocolActive) return; // Wenn Protokoll aus, dann nix anzeigen

            let data = JSON.parse(event.data);
            let row = document.createElement('tr');
            row.innerHTML = 
                "<td>" + data.pgmZeile + "</td>" +
                "<td>" + data.pgmFunc + "</td>" +
                "<td>" + data.timestamp + "</td>" +
                "<td>" + (data.millis || "-") + "</td>" +
                "<td>" + data.msg + "</td>" +
                "<td>" + data.varName + "</td>" +
                "<td>" + data.varValue + "</td>" +
                "<td>" + (data.watchdog ? "JA" : "nein") + "</td>" +
                "<td>" + (data.resetReason !== undefined ? data.resetReason : "-") + "</td>";
            logBody.appendChild(row);

            window.scrollTo(0, document.body.scrollHeight);
            if (data.fs_free_kb !== undefined && data.fs_free_percent !== undefined && data.fs_free_kb >= 0) {
                statusDiv.innerText = "Verbindung aktiv. Freier Speicher: " + data.fs_free_kb + " kB (" + data.fs_free_percent.toFixed(1) + "%)";
            }

        };
        conn.onclose = () => statusDiv.innerText = "Verbindung verloren.";

        // Button-Handler
        toggleBtn.addEventListener('click', () => {
            protocolActive = !protocolActive;
            if (protocolActive) {
                toggleBtn.textContent = "Protokoll AUS";
                mainTitle.textContent = "MYDBG WEB-Debug";
                mainTitle.style.color = "#0f0"; // grün
                if (conn.readyState === WebSocket.OPEN) conn.send("PROTOKOLL_EIN");
            } else {
                toggleBtn.textContent = "Protokoll EIN";
                mainTitle.textContent = "MYDBG WEB-Debug Protokoll AUS";
                mainTitle.style.color = "#f00"; // rot
                if (conn.readyState === WebSocket.OPEN) conn.send("PROTOKOLL_AUS");
            }
    });    
        document.getElementById('showJsonBtn').addEventListener('click', () => {
        window.open('/mydbg_data.json', '_blank');
    });
        document.getElementById('showWatchdogBtn').addEventListener('click', () => {
        window.open('/mydbg_watchdog.json', '_blank');
    });
    document.getElementById('deleteLogsBtn').addEventListener('click', () => {
        if (confirm('Willst du wirklich alle Logdateien löschen?')) {
            fetch('/delete_logs')
                .then(response => response.text())
                .then(text => {
                alert(text);
                // Tabelle leeren:
                logBody.innerHTML = "";
            })
                .catch(error => alert('Fehler beim Löschen: ' + error));
        }
    });
    </script>
</body>
</html>



        )rawliteral"); });

    // WebSocket Events
    MYDBG_ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                        void *arg, uint8_t *data, size_t len)
                     {
                         if (type == WS_EVT_CONNECT)
                         {
                             Serial.println("[MYDBG] WebSocket verbunden");
                             MYDBG_webClientActive = true;
                         }
                         else if (type == WS_EVT_DISCONNECT)
                         {
                             Serial.println("[MYDBG] WebSocket getrennt");
                             MYDBG_webClientActive = false;
                         }
                         else if (type == WS_EVT_DATA)
                         {
                             AwsFrameInfo *info = (AwsFrameInfo *)arg;
                             if (info->final && info->index == 0 && info->len == len)
                             {
                                 String msg = String((char *)data).substring(0, len);
                                 if (msg == "PROTOKOLL_AUS")
                                 {
                                     MYDBG_isEnabled = false;
                                     MYDBG_stopEnabled = false;
                                     Serial.println("[MYDBG] WebSocket-Befehl: Protokoll AUS empfangen.");
                                 }
                                 else if (msg == "PROTOKOLL_EIN")
                                 {
                                     MYDBG_isEnabled = true;
                                     MYDBG_stopEnabled = true;
                                     Serial.println("[MYDBG] WebSocket-Befehl: Protokoll EIN empfangen.");
                                 }
                             }
                         } });

    MYDBG_server.addHandler(&MYDBG_ws);
    MYDBG_server.begin();

    
}

// Zeitstempel (lokal oder [keine Zeit])
inline String MYDBG_getTimestamp()
{
    time_t now = time(nullptr);
    if (!MYDBG_timeInitDone || now < 1577836800)
    {
        if (!MYDBG_warnedAboutTime)
        {
            Serial.println("[MYDBG] ⚠️  Keine Zeit verfügbar (kein WLAN oder NTP fehlgeschlagen).");
            MYDBG_warnedAboutTime = true;
        }
        return "[keine Zeit]";
    }

    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char buf[30];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(buf);
}

// Zeitsynchronisation starten
inline void MYDBG_initTime(const char *ntpServer)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        // Kein WLAN – keine Zeit möglich
        return;
    }

    configTime(3600, 0, ntpServer); // z. B. UTC+1
    delay(1000);

    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        if (!MYDBG_timeInitDone)
        {
            // Nur einmal melden – vorher war keine Zeit da
            Serial.println("[MYDBG] ⏰ Zeit über NTP synchronisiert.");
        }
        MYDBG_timeInitDone = true;
    }
    else
    {
        // nur bei echtem Fehler
        Serial.println("[MYDBG] ⚠️ Zeit konnte nicht per NTP bezogen werden.");
        MYDBG_timeInitDone = false;
    }
}

    // Hilfsfunktionen für JSON-Logs
    void displayJsonLogs()
    {
    File file = LittleFS.open("/mydbg_data.json", "r");
    if (file)
    {
        Serial.println("\n=== Inhalt der JSON-Logdatei ===");
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error)
        {
            Serial.println("[MYDBG] Fehler beim Lesen der JSON-Datei.");
            return;
        }

        JsonArray logArray = doc["log"].as<JsonArray>();
        if (!logArray)
        {
            Serial.println("[MYDBG] Keine Logs gefunden.");
            return;
        }

        for (JsonObject entry : logArray)
        {
            // Kompakte Ausgabe in einer Zeile
            Serial.printf(
                "Zeit: %s | Millis: %ld | Funktion: %s |  Nachricht: %s | Variable: %s = %s | Reset: %d\n",
                "Zeit: %s | Millis: %ld | Funktion: %s |  Nachricht: %s | Variable: %s = %s | Reset: %d\n",
                entry["timestamp"].as<const char *>(),
                entry["millis"].as<long>(),
                entry["pgmFunc"].as<const char *>(),
                entry["pgmZeile"].as<int>(),
                entry["msg"].as<const char *>(),
                entry["varName"].as<const char *>(),
                entry["varValue"].as<const char *>(),
                entry["resetReason"].as<int>());
        }
        Serial.println("=== Ende der JSON-Logs ===\n");
    }
    else
    {
        Serial.println("[MYDBG] Keine Logdatei vorhanden.");
    }
}// Ende von displayJsonLogs()

// Löschen der JSON-Logs
void deleteJsonLogs()
{
    if (LittleFS.exists("/mydbg_data.json"))
    {
        LittleFS.remove("/mydbg_data.json");
        Serial.println("[MYDBG] /mydbg_data.json gelöscht.");
    }
    if (LittleFS.exists("/mydbg_watchdog.json"))
    {
        LittleFS.remove("/mydbg_watchdog.json");
        Serial.println("[MYDBG] /mydbg_watchdog.json gelöscht.");
    }
    if (LittleFS.exists("/mydbg_status.json"))
    {
        LittleFS.remove("/mydbg_status.json");
        Serial.println("[MYDBG] /mydbg_status.json gelöscht.");
    }
    Serial.println("\aJSON-Dateien gelöscht!\a");
}// Ende von deleteJsonLogs()


// Eingabe über Serial verarbeiten
void processSerialInput()
{
    unsigned long timeout = MYDBG_menuFirstCall ? 15000 : MYDBG_menuTimeout;
    MYDBG_menuFirstCall = false;

Serial.printf("> Eingabe (%lus Timeout) Menue # + CRLF: \n", timeout / 1000);
    unsigned long startMillis = millis();
    String input = "";

    // Warte auf Eingabe oder Timeout
    while (millis() - startMillis < timeout)
    {
        if (Serial.available() > 0)
        {
            char c = Serial.read();
            if (c == '\r' || c == '\n')
            {
                break; // Eingabe abgeschlossen
            }
            input += c;
        }
        delay(10); // Kurze Pause, um CPU-Last zu reduzieren
    }

    if (input.length() == 0)
    {
        Serial.println("Kein Eintrag – Timeout abgelaufen.");
        return;
    }

    Serial.println("Du hast eingegeben: " + input + "\n ");

    // Eingabe verarbeiten
    if (input == "1")
    {
        MYDBG_isEnabled = true;
        MYDBG_stopEnabled = true;
        MYDBG_setWatchdog(MYDBG_WDT_EXTENDED);
        Serial.println("[MYDBG] Modus 1 gesetzt: Debug AUSGABE + wait aktiv");
    }
    else if (input == "2")
    {
        MYDBG_isEnabled = true;
        MYDBG_stopEnabled = false;
        MYDBG_setWatchdog(MYDBG_WDT_EXTENDED);
        Serial.println("[MYDBG] Modus 2 gesetzt: Nur Debug AUSGABE aktiv");
    }
    else if (input == "3")
    {
        MYDBG_isEnabled = false;
        MYDBG_stopEnabled = false;
        MYDBG_setWatchdog(MYDBG_WDT_EXTENDED);
        Serial.println("[MYDBG] Modus 3 gesetzt: Debug AUSGABE + wait AUS");
    }
    else if (input == "4")
    {
        MYDBG_isEnabled = true;
        MYDBG_stopEnabled = true;
        MYDBG_setWatchdog(MYDBG_WDT_EXTENDED);
        MYDBG_webDebugEnabled = true;
        MYDBG_startWebDebug();

        Serial.println("\n[MYDBG] Modus 4 Web-Debug aktiv: http://" + WiFi.localIP().toString() + "/status.html");
        
        delay(3000);
    }
    else if (input == "5")
    {
        MYDBG_webDebugEnabled = false;
        Serial.println("[MYDBG] Modus 5 gesetzt: Web-Debug beendet");
    }
    else if (input == "6")
    {
        Serial.println("[MYDBG] Modus 6 gesetzt: JSON-Logs anzeigen (Serial-Ausgabe)");
        // JSON-Logs anzeigen
        displayJsonLogs();
    }
    else if (input == "7")
    {
        Serial.println("[MYDBG] Modus 7 gesetzt: Alle JSON-Logs löschen");
        deleteJsonLogs();
    }
    else
    {
        Serial.println("[MYDBG] Ungültige Eingabe: " + input);
    }
}// Ende von processSerialInput()

// Konsolenmenü für Debug-Einstellungen
inline void MYDBG_MENUE_IMPL(const char *aufruferFunc)
{
    if (!MYDBG_filesystemReady)
        MYDBG_initFilesystem();
    while (Serial.available())
        Serial.read(); // Eingabepuffer leeren

    Serial.println();
    Serial.println(String("=== MYDBG Menü (aufgerufen in: ") + aufruferFunc + ") ===");
    if (MYDBG_filesystemReady)
    {
        size_t total = LittleFS.totalBytes();
        size_t used = LittleFS.usedBytes();
        Serial.printf("Filesystem: %.2f kB verwendet, von %.2f kB (%.1f%% belegt)\n",
                      used / 1024.0, total / 1024.0, (used * 100.0) / total);
    }
    else
    {
        Serial.println("\n⚡ Achtung: Filesystem nicht bereit.\n");
        
    }

        Serial.println(MYDBG_isEnabled && MYDBG_stopEnabled ? "x 1 = Debug AUSGABE + wait aktiv" : "  1 = Debug AUSGABE + wait aktiv");
        Serial.println(MYDBG_isEnabled && !MYDBG_stopEnabled ? "x 2 = Nur Debug AUSGABE aktiv" : "  2 = Nur Debug AUSGABE aktiv");
        Serial.println(!MYDBG_isEnabled && !MYDBG_stopEnabled ? "x 3 = Debug AUSGABE + wait AUS" : "  3 = Debug AUSGABE + wait AUS");
        Serial.println(MYDBG_webDebugEnabled ? "x 4 = Web-Debug anzeigen (status.html)" : "  4 = Web-Debug anzeigen (status.html)");
        Serial.println(MYDBG_webDebugEnabled ? "  5 = Web-Debug beenden" : "x 5 = Web-Debug beenden");
        Serial.println("  6 = JSON-Logs anzeigen (Serial-Ausgabe)");
        Serial.println("  7 = Alle JSON-Logs löschen (Filesystem)\n");
        processSerialInput(); // Eingabe verarbeiten
}

#define MYDBG_MENUE() MYDBG_MENUE_IMPL(__FUNCTION__)

#endif // MYDBG_H