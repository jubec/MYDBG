// MYDBG Debug für Arduino-Code MfG jubec// Datei: MYDBG.h – main am 05.06.2025 _clean (angepasst für Port 56745)
/*
  MYDBG(…) – universelles Debug-Makro für Konsole + Web

  - MYDBG(0, "Nur Konsole")      → Nur serielle Ausgabe (schnell, ohne Delay)
  - MYDBG(5, "Stop & Web", val)  → Ausgabe auf Konsole + WebSocket + Log, mit Pause 5 Sek (optional 1-9 Sek.)

  Vorteile:
  - Mit „Suchen und Ersetzen“ kann MYDBG(0,… ) schnell in MYDBG(1,… ) umgewandelt werden und umgekehrt
  - Klarere Struktur als Serial.print() + delay()

  Empfehlung:
  - MYDBG(0, "ich bin hier") zur schnellen Orientierung
  - MYDBG(n, "Fehler bei x", var) zur Untersuchung inkl. Web-Log + JSON

  Verhalten:
  - Nur MYDBG(1…9) → Logeintrag, Webausgabe + Pause
  - MYDBG(0,…)     → nur Konsole, keine Netzlast

  Zusatzfunktionen:
  - MYDBG_displayJsonLogs()  → zeigt gespeicherte Logs aus JSON-Dateien
  - MYDBG_deleteJsonLogs()   → löscht /mydbg_*.json Dateien
  - MYDBG_resetJsonFiles()   → kombiniert Löschen + Wiederherstellung
  - MYDBG_MENUE()            → Konsolenmenü mit Steuerung der Debug-Parameter
*/

#ifndef MYDBG_H
#define MYDBG_H

#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <time.h>
#include <esp_task_wdt.h>
#include <ArduinoJson.h>

// === Systemeinstellungen ===
#define MYDBG_MAX_LOGFILES 10
#define MYDBG_MAX_WATCHDOGS 10
#define MYDBG_WDT_DEFAULT 10
#define MYDBG_WDT_EXTENDED 300

// === Debug-Flags ===
// #define MYDBG_NO_AUTOINIT             // unterdrückt automatische Initialisierung
// #define MYDBG_WEBDEBUG_NUR_MANUELL    // Web-Debug nur manuell starten
#define MYDBG_EIGENER_SERVER // Aktiviert eigenen Server für MYDBG

// === Statusvariablen ===
inline bool MYDBG_timeInitDone = false;
static bool MYDBG_warnedAboutTime = false;
inline bool MYDBG_isEnabled = true;
inline bool MYDBG_stopEnabled = true;
inline bool MYDBG_webDebugEnabled = false;
inline bool MYDBG_webClientActive = false;
inline bool MYDBG_filesystemReady = false;
inline bool MYDBG_menuFirstCall = true;
inline unsigned long MYDBG_menuTimeout = 5000;
static bool MYDBG_resetGrundExported = false;
static bool alreadyWritten = false;

// Vorwärtsdeklaration: JSON-Routen aktivieren
void MYDBG_addJsonRoutes(AsyncWebServer &server);

// void MYDBG_setupWebSocket(); // Funktion zum Einrichten des Debug-WebSocket

// === Globale Reset-Info ===
inline String MYDBG_resetGrundText = "";

// Initialisierung & Zeit
inline void MYDBG_autoInit();
inline void MYDBG_initFilesystem();
inline void MYDBG_initTime(const char *ntpServer = "pool.ntp.org");

// JSON-Dateien (Dateisystem)
inline void MYDBG_prepareJsonFiles();
inline void MYDBG_resetJsonFiles();
void MYDBG_deleteJsonLogs();
void MYDBG_displayJsonLogs();

// Debug-Ausgaben & Logging
inline String MYDBG_getTimestamp();
inline void MYDBG_stopAusgabe(const String &msg, const String &varName, const String &varValue, const String &func, int zeile);
inline void MYDBG_logToJson(const String &text, const String &func, int line, const String &varName, const String &varValue);
inline void MYDBG_writeStatusFile(const String &msg, const String &func, int line, const String &varName, const String &varValue);
inline void MYDBG_writeWatchdogRestartFromLastLog();
inline void MYDBG_fillResetInfo(JsonObject &doc, bool mitFarbe);

// WebSocket / Web Debug
inline void MYDBG_startWebDebug();
inline void MYDBG_streamWebLine(const String &msg);
inline void MYDBG_streamWebLineJSON(const String &msg, const String &varName, const String &varValue, const String &func, int zeile);

// Watchdog
inline void MYDBG_setWatchdog(int sekunden);

// Menüsystem
void MYDBG_MENUE();
#define MYDBG_MENUE() MYDBG_MENUE_IMPL(__FUNCTION__)

// === Webserver auf Port 56745 ===
#ifdef MYDBG_EIGENER_SERVER
AsyncWebServer MYDBG_server(56745); // Angepasster Port
AsyncWebSocket MYDBG_ws("/dbgws");
#else
extern AsyncWebServer server;
extern AsyncWebSocket MYDBG_ws;
#define MYDBG_server server
#endif

// Diese Struktur wird verwendet, um den Grund für den Reset zu interpretieren und eine Beschreibung sowie eine Farbe bereitzustellen.
struct MYDBG_ResetInfo
{
    const char *text;
    const char *farbe;
}; // Ende der Struktur MYDBG_ResetInfo

// Diese Funktion interpretiert den Resetgrund und gibt eine Struktur mit Text und Farbe zurück.
inline MYDBG_ResetInfo MYDBG_interpretResetReason(esp_reset_reason_t rsn)
{
    switch (rsn)
    {
    case ESP_RST_PANIC:
        return {"Kernel Panic", "#FF4444"}; // rot
    case ESP_RST_INT_WDT:
        return {"Interrupt Watchdog", "#FF6600"}; // orange
    case ESP_RST_TASK_WDT:
        return {"Task Watchdog", "#FF6600"}; // orange
    case ESP_RST_WDT:
        return {"Allgemeiner WDT", "#FF6600"}; // orange
    case ESP_RST_DEEPSLEEP:
        return {"Deep Sleep Wakeup", "#00AAAA"}; // blaugrün
    case ESP_RST_BROWNOUT:
        return {"Brownout (Spannung)", "#AA00AA"}; // violett
    case ESP_RST_SDIO:
        return {"SDIO Reset", "#CCCC00"}; // gelb
    case ESP_RST_SW:
        return {"Software Reset", "#AAAAAA"}; // grau
    case ESP_RST_EXT:
        return {"Externer Reset", "#8888FF"}; // hellblau
    case ESP_RST_POWERON:
        return {"Power-On Reset", "#44FF44"}; // grün
    default:
        return {"Unbekannt", "#888888"}; // grau
    }
} // Ende der Funktion MYDBG_interpretResetReason

// Diese Funktion initialisiert das Dateisystem und überprüft, ob es bereit ist.
inline void MYDBG_autoInit()
{
#ifndef MYDBG_NO_AUTOINIT
    static bool alreadyInitialized = false;

    if (!MYDBG_filesystemReady)
    {
        MYDBG_initFilesystem();
        MYDBG_prepareJsonFiles(); // Jetzt mit Fehlerprüfung und Reparatur
    }
    if (!MYDBG_timeInitDone)
        MYDBG_initTime(); // Gibt bereits bei Fehler selbstständig Warnung aus

    if (MYDBG_resetGrundText == "" && !MYDBG_resetGrundExported)
    {
        MYDBG_ResetInfo info = MYDBG_interpretResetReason(esp_reset_reason());
        MYDBG_resetGrundText = info.text;
        MYDBG_resetGrundExported = true;
    }

    MYDBG_writeWatchdogRestartFromLastLog(); // direkt nach Resetgrund-Ermittlung

#ifndef MYDBG_WEBDEBUG_NUR_MANUELL
    if (!MYDBG_webDebugEnabled)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            MYDBG_startWebDebug();
            MYDBG_webDebugEnabled = true;
            Serial.println("\n[MYDBG] Mit STRG anklicken = Webserver http://" + WiFi.localIP().toString() + ":56745/MYDBG_status.html");
            Serial.println("\n[MYDBG] Mit STRG anklicken = Statische Log-Data-Seite http://" + WiFi.localIP().toString() + ":56745/mydbg_data.json");
            Serial.println("\n[MYDBG] Mit STRG anklicken = Statische Watchdog-Seite http://" + WiFi.localIP().toString() + ":56745/mydbg_watchdog.json");
        }
        else
        {
            Serial.println("[MYDBG] ⚠️  Kein WLAN – Web-Debug vorerst deaktiviert.");
        }
    }
#endif

    alreadyInitialized = true; // ✅ wichtig
#endif

} // Ende der Funktion MYDBG_autoInit

// Makro für Debug-Ausgaben
#define MYDBG(...) MYDBG_WRAPPER(__VA_ARGS__, MYDBG3, MYDBG2)(__VA_ARGS__)
#define MYDBG_WRAPPER(_1, _2, _3, NAME, ...) NAME
#define MYDBG2(waitIndex, msgText) MYDBG_INTERNAL(waitIndex, msgText, "", "")
#define MYDBG3(waitIndex, msgText, var) MYDBG_INTERNAL(waitIndex, msgText, #var, String(var))

#define MYDBG_INTERNAL(waitIndex, msgText, varName, varValue)                            \
    do                                                                                   \
    {                                                                                    \
        MYDBG_autoInit();                                                                \
        if (MYDBG_isEnabled && (!MYDBG_stopEnabled || waitIndex == 0))                   \
        {                                                                                \
            String meldung = String("[MYDBG] > ") + __LINE__ + " | " + msgText;          \
            if (varName != "" && varValue != "")                                         \
            {                                                                            \
                meldung += " | " + String(varName) + " = " + String(varValue);           \
            }                                                                            \
            Serial.println(meldung);                                                     \
        }                                                                                \
        if (waitIndex > 0 && MYDBG_stopEnabled)                                          \
        {                                                                                \
            int ms = constrain(waitIndex * 1000, 0, 9000);                               \
            MYDBG_stopAusgabe(msgText, varName, varValue, __FUNCTION__, __LINE__);       \
            if (!MYDBG_filesystemReady)                                                  \
                MYDBG_initFilesystem();                                                  \
            MYDBG_logToJson(msgText, __FUNCTION__, __LINE__, varName, varValue);         \
            MYDBG_writeStatusFile(msgText, __FUNCTION__, __LINE__, varName, varValue);   \
            MYDBG_streamWebLineJSON(msgText, varName, varValue, __FUNCTION__, __LINE__); \
            unsigned long __t0 = millis();                                               \
            while (millis() - __t0 < ms)                                                 \
            {                                                                            \
                delay(10);                                                               \
            }                                                                            \
        }                                                                                \
    } while (0) // Ende des Makros MYDBG_INTERNAL

// diese Funktion gibt den Resetgrund aus
inline void MYDBG_fillResetInfo(JsonObject &doc, bool mitFarbe = true)
{
    esp_reset_reason_t rsn = esp_reset_reason();
    MYDBG_ResetInfo info = MYDBG_interpretResetReason(rsn);
    doc["resetReason"] = (int)rsn;
    doc["ResetGrund"] = info.text;
    if (mitFarbe)
        doc["ResetColor"] = info.farbe;
} // Ende der Funktion MYDBG_fillResetInfo

// Speichert den Watchdog mit dem MYDBUG(x, ...) vor dem Watchdog-Reset
inline void MYDBG_writeWatchdogRestartFromLastLog()
{
    
    if (alreadyWritten)
        return;
    alreadyWritten = true;

    if (!LittleFS.exists("/mydbg_data.json"))
        return;

    File file = LittleFS.open("/mydbg_data.json", "r");
    if (!file)
        return;

    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error || !doc["log"].is<JsonArray>() || doc["log"].as<JsonArray>().size() == 0)
        return;

    JsonObject lastEntry = doc["log"][0];

    // Watchdog-Log vorbereiten
    StaticJsonDocument<2048> wdDoc;
    if (LittleFS.exists("/mydbg_watchdog.json"))
    {
        File f = LittleFS.open("/mydbg_watchdog.json", "r");
        if (f)
        {
            deserializeJson(wdDoc, f);
            f.close();
        }
    }

    JsonArray wdArr = wdDoc["watchdogs"].is<JsonArray>() ? wdDoc["watchdogs"].as<JsonArray>() : wdDoc.createNestedArray("watchdogs");

    JsonObject copy = wdArr.createNestedObject();
    for (JsonPair kv : lastEntry)
        copy[kv.key()] = kv.value();

    esp_reset_reason_t rsn = esp_reset_reason();
    MYDBG_ResetInfo info = MYDBG_interpretResetReason(rsn);
    copy["ResetGrund"] = info.text;
    copy["reason"] = (int)rsn;

    // Markiere kritische Resetarten
    switch (rsn)
    {
    case ESP_RST_PANIC:
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:
    case ESP_RST_BROWNOUT:
        copy["kritisch"] = true;
        break;
    default:
        copy["kritisch"] = false;
        break;
    }

    while (wdArr.size() > MYDBG_MAX_WATCHDOGS)
        wdArr.remove(wdArr.size() - 1);

    File out = LittleFS.open("/mydbg_watchdog.json", "w");
    if (out)
    {
        serializeJson(wdDoc, out);
        out.close();
    }
} // Ende der Funktion MYDBG_writeWatchdogRestartFromLastLog

// Einfache Textzeile an WebClient senden
inline void MYDBG_streamWebLineJSON(const String &msg, const String &varName, const String &varValue, const String &func, int zeile)
{
    StaticJsonDocument<256> doc;
    JsonObject root = doc.to<JsonObject>(); // <== wichtig!

    root["timestamp"] = MYDBG_getTimestamp();
    root["pgmFunc"] = func;
    root["pgmZeile"] = zeile;
    root["msg"] = msg;
    root["varName"] = varName;
    root["varValue"] = varValue;
    root["millis"] = millis();

    MYDBG_fillResetInfo(root, true); // Farbe nur fürs Web nötig

    if (MYDBG_filesystemReady)
    {
        size_t total = LittleFS.totalBytes();
        size_t used = LittleFS.usedBytes();
        float freiProzent = 100.0 - (used * 100.0) / total;
        root["fs_free_kb"] = (total - used) / 1024;
        root["fs_free_percent"] = freiProzent;
    }
    else
    {
        root["fs_free_kb"] = -1;
        root["fs_free_percent"] = -1;
    }

    String jsonStr;
    serializeJson(doc, jsonStr);
    MYDBG_ws.textAll(jsonStr);
} // Ende der Funktion MYDBG_streamWebLineJSON

// diese Funktion prüft JsonFiles
inline void MYDBG_prepareJsonFiles()
{
    struct JsonInitFile
    {
        const char *pfad;
        const char *grundinhalt; // Muss gültiges JSON sein!
    };

    const JsonInitFile dateien[] = {
        {"/mydbg_data.json", "{\"log\":[]}"},
        {"/mydbg_watchdog.json", "{\"watchdogs\":[]}"},
        {"/mydbg_status.json", "{}"}};

    for (const auto &eintrag : dateien)
    {
        bool dateiErsetzen = false;

        if (LittleFS.exists(eintrag.pfad))
        {
            File testfile = LittleFS.open(eintrag.pfad, "r");
            if (testfile)
            {
                StaticJsonDocument<256> testdoc;
                DeserializationError error = deserializeJson(testdoc, testfile);
                testfile.close();
                if (error)
                {
                    Serial.printf("[MYDBG] ⚠️ JSON-Fehler in %s: %s → wird ersetzt\n", eintrag.pfad, error.c_str());
                    dateiErsetzen = true;
                }
                else
                {
                    Serial.printf("[MYDBG] OK: %s ist gültig.\n", eintrag.pfad);
                }
            }
            else
            {
                Serial.printf("[MYDBG] ❌ Fehler beim Öffnen von %s\n", eintrag.pfad);
                dateiErsetzen = true;
            }
        }
        else
        {
            Serial.printf("[MYDBG] ⏺️  %s fehlt – wird angelegt\n", eintrag.pfad);
            dateiErsetzen = true;
        }

        if (dateiErsetzen)
        {
            File neu = LittleFS.open(eintrag.pfad, "w");
            if (neu)
            {
                neu.print(eintrag.grundinhalt);
                neu.close();
                Serial.printf("[MYDBG] ✅ %s wurde neu erstellt.\n", eintrag.pfad);
            }
            else
            {
                Serial.printf("[MYDBG] ❌ Fehler beim Erstellen von %s\n", eintrag.pfad);
            }
        }
    }
} // Ende der Funktion MYDBG_prepareJsonFiles

// LittleFS initialisieren
inline void MYDBG_initFilesystem()
{
    if (!LittleFS.begin(true))
    {
        Serial.println("[MYDBG] ⚠️ Fehler beim Mounten von LittleFS!");
    }
    MYDBG_filesystemReady = true;
} // Ende der Funktion MYDBG_initFilesystem

// Watchdog aktivieren
inline void MYDBG_setWatchdog(int sekunden)
{
    esp_task_wdt_init(sekunden, true);
    esp_task_wdt_add(NULL);
} // Ende der Funktion MYDBG_setWatchdog

// STOP-Ausgabe über Serial und Web
inline void MYDBG_stopAusgabe(const String &msg, const String &varName, const String &varValue, const String &func, int zeile)
{
    if (!MYDBG_isEnabled)
        return;
    String ausgabe = "[MYDBG] > " + String(zeile) + " | " + func + "() | " + MYDBG_getTimestamp() + " | " + millis() + " | " + msg + " | " + varName + " = " + varValue;
    Serial.println(ausgabe);
    MYDBG_streamWebLine(ausgabe);
} // Ende der Funktion MYDBG_stopAusgabe

// gibt webseite aus
inline void MYDBG_streamWebLine(const String &msg)
{
    if (MYDBG_webClientActive)
    {
        MYDBG_ws.textAll(msg);
    }
} // Ende der Funktion MYDBG_streamWebLine

// Log-Eintrag in JSON-Datei schreiben
// Neue Version von MYDBG_logToJson() mit sauberem LIFO und echten Objektkopien
inline void MYDBG_logToJson(const String &text, const String &func, int line, const String &varName, const String &varValue)
{
    bool isWatchdogReset = (esp_reset_reason() == ESP_RST_WDT || esp_reset_reason() == ESP_RST_TASK_WDT);

    StaticJsonDocument<2048> doc;

    if (LittleFS.exists("/mydbg_data.json"))
    {
        File file = LittleFS.open("/mydbg_data.json", "r");
        if (file)
        {
            DeserializationError error = deserializeJson(doc, file);
            file.close();
            if (error)
                doc.clear();
        }
    }

    JsonArray oldArr = doc["log"].is<JsonArray>() ? doc["log"].as<JsonArray>() : doc.createNestedArray("log");

    // Neues Dokument vorbereiten
    StaticJsonDocument<2048> newDoc;
    JsonArray newArr = newDoc.createNestedArray("log");

    JsonObject newEntry = newArr.createNestedObject();
    newEntry["timestamp"] = MYDBG_getTimestamp();
    newEntry["millis"] = millis();
    newEntry["pgmFunc"] = func;
    newEntry["pgmZeile"] = line;
    newEntry["msg"] = text;
    newEntry["varName"] = varName;
    newEntry["varValue"] = varValue;
    MYDBG_fillResetInfo(newEntry, false); // ohne Farbe für JSON-Datei

    for (JsonObject o : oldArr)
    {
        if (newArr.size() >= MYDBG_MAX_LOGFILES)
            break;
        JsonObject copy = newArr.createNestedObject();
        for (JsonPair kv : o)
        {
            copy[kv.key()] = kv.value();
        }
    }

    File out = LittleFS.open("/mydbg_data.json", "w");
    if (out)
    {
        serializeJson(newDoc, out);
        out.close();
    }
} // Ende der Funktion MYDBG_logToJson

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
} // Ende der Funktion MYDBG_writeStatusFile

// JSON-Ausgabe der Logs über Webserver bereitstellen
inline void MYDBG_addJsonRoutes(AsyncWebServer &server)
{
    server.on("/mydbg_data.json", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        if (LittleFS.exists("/mydbg_data.json"))
        {
            request->send(LittleFS, "/mydbg_data.json", "application/json");
        }
        else
        {
            request->send(404, "application/json", "{\"error\":\"Keine Logdatei gefunden\"}");
        } });

    server.on("/mydbg_watchdog.json", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        if (LittleFS.exists("/mydbg_watchdog.json"))
        {
            request->send(LittleFS, "/mydbg_watchdog.json", "application/json");
        }
        else
        {
            request->send(404, "application/json", "{\"error\":\"Keine Watchdog-Datei gefunden\"}");
        } });
    server.on("/mydbg_delete_logs", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        MYDBG_resetJsonFiles();
    request->send(200, "text/plain", "Logdateien wurden gelöscht und neu vorbereitet."); });
} // Ende der Funktion MYDBG_addJsonRoutes

// Web-Debug-Seite starten
inline void MYDBG_startWebDebug()
{
    MYDBG_addJsonRoutes(MYDBG_server); // JSON-Routen für /mydbg_data.json und /mydbg_watchdog.json aktivieren
    // HTTP-Handler für MYDBG_status.html
    MYDBG_server.on("/MYDBG_status.html", HTTP_GET, [](AsyncWebServerRequest *request)
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
            th { background: #003300; color: #ccffcc; font-weight: bold; position: sticky; top: 160px; z-index: 5; }
            tr:nth-child(even) { background: #000; }
            #status, #mydbg_resetGrund { margin: 20px; font-size: 1.2em;font-weight: bold; color: #ccc; }
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
            <div id="mydbg_resetGrund">Letzter Reset: unbekannt</div>
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
                </tr>
            </thead>
            <tbody id="logBody"></tbody>
        </table>

    <script>
    const WEBSOCKET_TIMEOUT_MS = 20000; // 20 Sekunden
    let conn;
    let retryDelay = 3000;
    let statusDiv = document.getElementById('status');
    let resetDiv = document.getElementById('mydbg_resetGrund');
    let logBody = document.getElementById('logBody');
    let mainTitle = document.getElementById('mainTitle');
    let toggleBtn = document.getElementById('toggleProtocolBtn');
    let protocolActive = true;
    let lastMessageTime = Date.now();

    function setVerbindungsStatus(ok) {
        if (ok) {
            statusDiv.innerText = "✅ Verbindung aktiv.";
            statusDiv.style.color = "#0f0";
            document.title = "✅ MYDBG verbunden";
        } else {
            statusDiv.innerText = "❌ Keine Verbindung!";
            statusDiv.style.color = "#f00";
            document.title = "❌ MYDBG getrennt";
        }
    }

    function handleMessage(event) {
        lastMessageTime = Date.now();
        if (!protocolActive) return;

        let data = JSON.parse(event.data);
        let row = document.createElement('tr');
        row.innerHTML = 
            "<td>" + data.pgmZeile + "</td>" +
            "<td>" + data.pgmFunc + "</td>" +
            "<td>" + data.timestamp + "</td>" +
            "<td>" + (data.millis || "-") + "</td>" +
            "<td>" + data.msg + "</td>" +
            "<td>" + data.varName + "</td>" +
            "<td>" + data.varValue + "</td>";

        logBody.insertBefore(row, logBody.firstChild);

        if (data.fs_free_kb !== undefined && data.fs_free_percent !== undefined && data.fs_free_kb >= 0) {
            statusDiv.innerText = "✅ Verbindung aktiv. Freier Speicher: " + data.fs_free_kb + " kB (" + data.fs_free_percent.toFixed(1) + "%)";
        }

        if (data.resetReason !== undefined && data.resetReason > 0) {
            const grundText = data.ResetGrund || interpretResetReason(data.resetReason);
            const color = data.ResetColor || "#ccc";
            resetDiv.innerText = "Letzter Reset: " + grundText;
            resetDiv.style.color = color;
        }
    }

    function interpretResetReason(code) {
        const reasons = {
            1: "PowerOn",
            2: "ExtReset",
            3: "SW-Reset",
            4: "Panic",
            5: "Int-WDT",
            6: "Task-WDT",
            7: "WDT",
            8: "DeepSleep",
            9: "Brownout",
            10: "SDIO",
            11: "RTC Watchdog",
            12: "Unknown Reset"
        };
        return reasons[code] || "Unbekannt (" + code + ")";
    }

    function startWebSocket() {
        conn = new WebSocket('ws://' + location.host + '/dbgws');

        conn.onopen = () => {
            setVerbindungsStatus(true);
            console.log("WebSocket verbunden");
        };

        conn.onmessage = handleMessage;

        conn.onclose = () => {
            setVerbindungsStatus(false);
            console.warn("WebSocket getrennt – neuer Versuch in " + retryDelay + "ms");
            setTimeout(startWebSocket, retryDelay);
        };

        conn.onerror = (err) => {
            console.error("WebSocket Fehler:", err);
            conn.close(); // erzwingt reconnect über onclose
        };
    }

    // Lebenszeichen überwachen – wird jede 3s geprüft
    setInterval(() => {
        if (Date.now() - lastMessageTime > WEBSOCKET_TIMEOUT_MS) { 
            console.warn("⚠️ Keine Daten seit 20s – Erzwinge reconnect");
            if (conn && conn.readyState === WebSocket.OPEN) {
                conn.close(); // triggert onclose und reconnect
            }
        }
    }, 3000);

    startWebSocket();

    document.getElementById('showJsonBtn').addEventListener('click', () => {
        window.open('/mydbg_data.json', '_blank');
    });
    document.getElementById('showWatchdogBtn').addEventListener('click', () => {
        window.open('/mydbg_watchdog.json', '_blank');
    });
    document.getElementById('deleteLogsBtn').addEventListener('click', () => {
        if (confirm('Willst du wirklich alle Logdateien löschen?')) {
            fetch('/mydbg_delete_logs')
                .then(response => response.text())
                .then(text => {
                    alert(text);
                    logBody.innerHTML = "";
                })
                .catch(error => alert('Fehler beim Löschen: ' + error));
        }
    });
    toggleBtn.addEventListener('click', () => {
        protocolActive = !protocolActive;
        if (protocolActive) {
            toggleBtn.textContent = "Protokoll AUS";
            mainTitle.textContent = "MYDBG WEB-Debug";
            mainTitle.style.color = "#0f0";
            if (conn.readyState === WebSocket.OPEN) conn.send("PROTOKOLL_EIN");
        } else {
            toggleBtn.textContent = "Protokoll EIN";
            mainTitle.textContent = "MYDBG WEB-Debug Protokoll AUS";
            mainTitle.style.color = "#f00";
            if (conn.readyState === WebSocket.OPEN) conn.send("PROTOKOLL_AUS");
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
#ifdef MYDBG_EIGENER_SERVER
    MYDBG_server.begin();
#endif
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
} // Ende der Funktion MYDBG_getTimestamp

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
} // Ende der Funktion MYDBG_initTime

// Hilfsfunktionen für JSON-Logs
void MYDBG_displayJsonLogs()
{
    struct LogFileInfo
    {
        const char *pfad;
        const char *titel;
        const char *arrayKey;
    };

    const LogFileInfo dateien[] = {
        {"/mydbg_data.json", "JSON-Logdatei", "log"},
        {"/mydbg_watchdog.json", "Watchdog-Logdatei", "watchdogs"}};

    for (const auto &datei : dateien)
    {
        Serial.printf("\n=== Inhalt der %s ===\n", datei.titel);
        if (!LittleFS.exists(datei.pfad))
        {
            Serial.printf("[MYDBG] Datei fehlt: %s\n", datei.pfad);
            continue;
        }

        File file = LittleFS.open(datei.pfad, "r");
        if (!file)
        {
            Serial.printf("[MYDBG] Fehler beim Öffnen: %s\n", datei.pfad);
            continue;
        }

        StaticJsonDocument<2048> doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error || !doc[datei.arrayKey].is<JsonArray>())
        {
            Serial.printf("[MYDBG] Fehler beim Parsen oder Array fehlt: %s\n", datei.pfad);
            continue;
        }

        JsonArray arr = doc[datei.arrayKey].as<JsonArray>();
        for (JsonObject e : arr)
        {
            Serial.printf("Zeit: %s | Funktion: %s | Zeile: %d | Nachricht: %s | Variable: %s = %s | Reset: %d\n",
                          e["timestamp"] | "[?]",
                          e["pgmFunc"] | "?",
                          e["pgmZeile"] | -1,
                          e["msg"] | "?",
                          e["varName"] | "-",
                          e["varValue"] | "-",
                          e["resetReason"] | -1);
        }
    }

    Serial.println("\n=== Ende aller Logs ===\n");
} // Ende der Funktion MYDBG_displayJsonLogs

// Löschen und Neuinitialisieren der JSON-Logs
inline void MYDBG_resetJsonFiles()
{
    MYDBG_deleteJsonLogs();
    MYDBG_prepareJsonFiles();
    Serial.println("[MYDBG] ✅ JSON-Dateien wurden gelöscht und neu vorbereitet.");
} // Ende der Funktion MYDBG_resetJsonFiles

// Löschen der JSON-Logs
void MYDBG_deleteJsonLogs()
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
} // Ende von MYDBG_deleteJsonLogs()

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
        delay(10);
    }

    if (input.length() == 0)
    {
        Serial.println("Kein Eintrag – Timeout abgelaufen.");
        return;
    }

    Serial.println("Du hast eingegeben: " + input + "\n");

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
        if (!MYDBG_webDebugEnabled)
        {
            MYDBG_webDebugEnabled = true;
            MYDBG_startWebDebug();
            Serial.println("[MYDBG] Modus 4: Web-Debug aktiviert: http://" + WiFi.localIP().toString() + "/MYDBG_status.html");
        }
        else
        {
            Serial.println("[MYDBG] Web-Debug ist bereits aktiv.");
        }
    }
    else if (input == "5")
    {
        MYDBG_webDebugEnabled = false;
        Serial.println("[MYDBG] Modus 5 gesetzt: Web-Debug deaktiviert (Standardzustand)");
    }
    else if (input == "6")
    {
        Serial.println("[MYDBG] Modus 6 gesetzt: JSON-Logs anzeigen (Serial-Ausgabe)");
        MYDBG_displayJsonLogs();
    }
    else if (input == "7")
    {
        Serial.println("[MYDBG] Modus 7 gesetzt: Alle JSON-Logs löschen und neu vorbereiten");
        MYDBG_resetJsonFiles();
    }
    else
    {
        Serial.println("[MYDBG] Ungültige Eingabe: " + input);
    }
} // Ende der Funktion processSerialInput

// Konsolenmenü für Debug-Einstellungen
inline void MYDBG_MENUE_IMPL(const char *aufruferFunc)
{
    if (!MYDBG_filesystemReady)
        MYDBG_initFilesystem();
    while (Serial.available())
        Serial.read(); // Eingabepuffer leeren

    Serial.println();
    Serial.println(String("=== MYDBG Menü (aufgerufen in: ") + aufruferFunc + ") ===");

    static bool startInfoGezeigt = false;
    if (!startInfoGezeigt)
    {
        Serial.println("\n=== Systemeinstellungen (nur beim ersten Menüaufruf) ===");
        Serial.printf("[MYDBG] MAX_LOGFILES     = %d   >>> Maximale Anzahl gespeicherter Logeinträge\n", MYDBG_MAX_LOGFILES);
        Serial.printf("[MYDBG] MAX_WATCHDOGS    = %d   >>> Maximale Anzahl gespeicherter Watchdog-Einträge\n", MYDBG_MAX_WATCHDOGS);
        Serial.printf("[MYDBG] WDT_DEFAULT      = %d   >>> Standard-Watchdog in Sekunden (bei Debug-Stop)\n", MYDBG_WDT_DEFAULT);
        Serial.printf("[MYDBG] WDT_EXTENDED     = %d   >>> Erweiterter Watchdog bei Benutzerwahl (z. B. Menü)\n", MYDBG_WDT_EXTENDED);
#ifdef MYDBG_NO_AUTOINIT
        Serial.println("[MYDBG] MYDBG_NO_AUTOINIT ist AKTIV – Automatische Initialisierung ist deaktiviert");
#else
        Serial.println("[MYDBG] Automatische Initialisierung ist AKTIV (kein MYDBG_NO_AUTOINIT)");
#endif
#ifdef MYDBG_WEBDEBUG_NUR_MANUELL
        Serial.println("[MYDBG] MYDBG_WEBDEBUG_NUR_MANUELL ist AKTIV – Web-Debug muss manuell gestartet werden");
#else
        Serial.println("[MYDBG] Web-Debug wird automatisch gestartet (kein MYDBG_WEBDEBUG_NUR_MANUELL)");
#endif

        Serial.println("[MYDBG] WLAN-Status: " + String(WiFi.status() == WL_CONNECTED ? "verbunden mit : " + String(WiFi.SSID()) : "nicht verbunden"));

        Serial.println("[MYDBG] Web-Debug Status: http://" + WiFi.localIP().toString() + "/MYDBG_status.html");
        Serial.println("[MYDBG] Web-Debug Daten: http://" + WiFi.localIP().toString() + "/mydbg_data.jso n");
        Serial.println("[MYDBG] Web-Debug Watchdog: http://" + WiFi.localIP().toString() + "/mydbg_watchdog.json");
        Serial.println("[MYDBG] Web-Debug: http://" + WiFi.localIP().toString() + "/mydbg_delete_logs");

        Serial.println("=============================================================================\n");
        startInfoGezeigt = true;
    }

    // ResetGrund gleich am Anfang anzeigen
    esp_reset_reason_t rsn = esp_reset_reason();
    MYDBG_ResetInfo info = MYDBG_interpretResetReason(rsn);

    Serial.printf("Letzter Reset: %s (Code %d)\n", info.text, rsn);

    // Optional: deutliches Warnsignal bei Watchdog
    if (rsn == ESP_RST_WDT || rsn == ESP_RST_TASK_WDT || rsn == ESP_RST_INT_WDT)
    {
        Serial.println("⚠️  ⚠️  ⚠️  Watchdog-Reset erkannt! ⚠️  ⚠️  ⚠️");
    }

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
    Serial.println(MYDBG_webDebugEnabled ? "x 4 = Web-Debug anzeigen (MYDBG_status.html)" : "  4 = Web-Debug anzeigen (MYDBG_status.html)");
    Serial.println(!MYDBG_webDebugEnabled ? "x 5 = Web-Debug beenden (Standard)" : "  5 = Web-Debug beenden (Standard)");
    Serial.println("  6 = JSON-Logs anzeigen (Serial-Ausgabe)");
    Serial.println("  7 = Alle JSON-Logs löschen (Filesystem)\n");
    processSerialInput(); // Eingabe verarbeiten
} // Ende der Funktion MYDBG_MENUE_IMPL

#define MYDBG_MENUE() MYDBG_MENUE_IMPL(__FUNCTION__)

#endif // MYDBG_H