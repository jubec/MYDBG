// Datei: MYDBG.h – vollständige Debug-Headerdatei (Stand April 2025)
#ifndef MYDBG_H
#define MYDBG_H

#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <time.h>
#include <esp_task_wdt.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>


#define MYDBG_MAX_LOGFILES 3
#define MYDBG_WDT_DEFAULT 10
#define MYDBG_WDT_EXTENDED 300

inline bool MYDBG_timeInitDone = false;
inline bool MYDBG_isEnabled = true;
inline bool MYDBG_stopEnabled = true;
inline bool MYDBG_webDebugEnabled = false;
inline bool MYDBG_webClientActive = false;
inline bool MYDBG_filesystemReady = false;
inline bool MYDBG_menuFirstCall = true;
inline unsigned long MYDBG_menuTimeout = 5000;
inline int MYDBG_MAX_WATCHDOGS = 10; // Maximal 10 Watchdog-Einträge (einstellbar)
inline bool protokollAktiv = true;

AsyncWebServer MYDBG_server(80);
AsyncWebSocket MYDBG_ws("/ws");

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

const char MYDBG_HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8">
  <title>MYDBG Web-Debug</title>
  <style>
    body { font-family: monospace; background-color: #e0ffe0; padding: 20px; }
    h1, h2 { color: #006600; }
    pre { background: #ffffff; padding: 10px; border: 1px solid #006600; overflow-x: auto; }
    button { margin-top: 10px; padding: 5px 10px; font-size: 16px; }
    #status { font-size: 24px; margin: 20px 0; }
    .off { color: red; font-weight: bold; }
    .on { color: green; font-weight: bold; }
    #serverResponse { margin-top: 20px; font-weight: bold; color: blue; }
  </style>
</head>
<body>

<h1>MYDBG Web-Debug</h1>

<div id="status" class="on">Protokoll AN</div>

<button onclick="reloadLog()">Log neu laden</button>
<button onclick="reloadWatchdog()">Watchdog neu laden</button>
<button onclick="deleteLogs()">Logs löschen</button>
<button onclick="enableProtocol()">Protokoll AN</button>
<button onclick="disableProtocol()">Protokoll AUS</button>

<div id="serverResponse"></div>

<h2>Laufende Live-Stopps:</h2>
<div id="live" style="white-space: pre-wrap; background: #ccffcc; padding: 10px; border: 1px solid #006600;"></div>

<h2>Aktuelle Log-Liste:</h2>
<pre id="logdata">Laden...</pre>

<h2>Watchdog-Liste:</h2>
<pre id="watchdogdata">Laden...</pre>

<script>
  const ws = new WebSocket(`ws://${location.hostname}/ws`);
  ws.onmessage = (event) => {
    document.getElementById('live').textContent += event.data + "\\n";
  };

  function reloadLog() {
    fetch('/mydbg_data.json')
      .then(response => response.json())
      .then(data => {
        document.getElementById('logdata').textContent = JSON.stringify(data, null, 2);
      })
      .catch(error => {
        document.getElementById('logdata').textContent = "Fehler beim Laden der Logdaten!";
      });
  }

  function reloadWatchdog() {
    fetch('/mydbg_watchdog.json')
      .then(response => response.json())
      .then(data => {
        document.getElementById('watchdogdata').textContent = JSON.stringify(data, null, 2);
      })
      .catch(error => {
        document.getElementById('watchdogdata').textContent = "Fehler beim Laden der Watchdogdaten!";
      });
  }

  function deleteLogs() {
    fetch('/deleteLogs')
      .then(() => {
        document.getElementById('serverResponse').textContent = "Logs gelöscht";
        reloadLog();
        reloadWatchdog();
      })
      .catch(() => {
        document.getElementById('serverResponse').textContent = "Fehler beim Löschen der Logs!";
      });
  }

  function enableProtocol() {
    fetch('/enableProtocol')
      .then(() => {
        document.getElementById('status').textContent = "Protokoll AN";
        document.getElementById('status').className = "on";
        document.getElementById('serverResponse').textContent = "Protokollierung AN";
      });
  }

  function disableProtocol() {
    fetch('/disableProtocol')
      .then(() => {
        document.getElementById('status').textContent = "Protokoll AUS";
        document.getElementById('status').className = "off";
        document.getElementById('serverResponse').textContent = "Protokollierung AUS";
      });
  }

  function checkProtocolState() {
    fetch('/getProtocolState')
      .then(response => response.text())
      .then(state => {
        if (state.trim() === "AN") {
          document.getElementById('status').textContent = "Protokoll AN";
          document.getElementById('status').className = "on";
        } else {
          document.getElementById('status').textContent = "Protokoll AUS";
          document.getElementById('status').className = "off";
        }
      })
      .catch(error => {
        document.getElementById('status').textContent = "Status unbekannt";
        document.getElementById('status').className = "off";
      });
  }

  // Direkt beim Start laden
  reloadLog();
  reloadWatchdog();
  checkProtocolState();
</script>

</body>
</html>
)rawliteral";

#define MYDBG(...) MYDBG_WRAPPER(__VA_ARGS__, MYDBG3, MYDBG2)(__VA_ARGS__)
#define MYDBG_WRAPPER(_1, _2, _3, NAME, ...) NAME
#define MYDBG2(waitIndex, msgText) MYDBG_INTERNAL(waitIndex, msgText, "---", "---")
#define MYDBG3(waitIndex, msgText, var) MYDBG_INTERNAL(waitIndex, msgText, #var, String(var))

#define MYDBG_INTERNAL(waitIndex, msgText, varName, varValue)                                                       \
    do                                                                                                              \
    {                                                                                                               \
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
                delay(10);                                                                                          \
            }                                                                                                       \
        }                                                                                                           \
    } while (0)

// JSON-Nachricht per WebSocket senden
inline void MYDBG_streamWebLineJSON(const String &msg, const String &varName, const String &varValue, const String &func, int zeile)
{
    if (!MYDBG_webDebugEnabled || !MYDBG_webClientActive)
        return;

    StaticJsonDocument<256> doc;
    doc["timestamp"] = MYDBG_getTimestamp();
    doc["pgmFunc"] = func;
    doc["pgmZeile"] = zeile;
    doc["msg"] = msg;
    doc["varName"] = varName;
    doc["varValue"] = varValue;

    String jsonStr;
    serializeJson(doc, jsonStr);

    MYDBG_ws.textAll(jsonStr);
}
// Einfache Textzeile an WebClient senden
inline void MYDBG_streamWebLine(const String &msg)
{
    
}

// LittleFS initialisieren
inline void MYDBG_initFilesystem()
{
    if (!LittleFS.begin(true))
        Serial.println("[MYDBG] Fehler beim Mounten von LittleFS!");
    else
        Serial.println("[MYDBG] LittleFS erfolgreich gemountet.");
    MYDBG_filesystemReady = true;
}
// Hilfsfunktion für Watchdog-Kennung
inline String MYDBG_getWatchdogText()
{
    return "Watchdog Reset erkannt"; // Später kann man hier dynamisch erweitern!
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
inline void MYDBG_logToWatchdog(const JsonObject &lastEntry)
{
    StaticJsonDocument<2048> wdDoc;
    File wdFile = LittleFS.open("/mydbg_watchdog.json", "r");
    if (wdFile)
    {
        DeserializationError error = deserializeJson(wdDoc, wdFile);
        wdFile.close();
        if (error)
            wdDoc.clear();
    }

    JsonArray wdArr;
    if (!wdDoc["watchdogs"].is<JsonArray>())
        wdArr = wdDoc.createNestedArray("watchdogs");
    else
        wdArr = wdDoc["watchdogs"].as<JsonArray>();

    while (wdArr.size() >= MYDBG_MAX_WATCHDOGS)
        wdArr.remove(0);

    JsonObject w = wdArr.add<JsonObject>();
    w["pgmZeile"] = lastEntry["pgmZeile"];
    w["pgmFunc"] = lastEntry["pgmFunc"];
    w["timestamp"] = lastEntry["timestamp"];
    w["millis"] = lastEntry["millis"];
    w["msg"] = lastEntry["msg"];
    w["varName"] = lastEntry["varName"];
    w["varValue"] = lastEntry["varValue"];
    w["watchdogCode"] = lastEntry["watchdogCode"];
    w["watchdogText"] = lastEntry["watchdogText"];

    File out = LittleFS.open("/mydbg_watchdog.json", "w");
    if (out)
    {
        serializeJson(wdDoc, out);
        out.close();
    }
}


// Log-Eintrag in JSON-Datei schreiben
inline void MYDBG_logToJson(const String &text, const String &func, int line, const String &varName, const String &varValue)
{
    bool isWatchdogReset = (esp_reset_reason() == ESP_RST_WDT);

    StaticJsonDocument<2048> doc;
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
    entry["pgmZeile"] = line;
    entry["pgmFunc"] = func;
    entry["timestamp"] = MYDBG_getTimestamp();
    entry["millis"] = millis();
    entry["msg"] = text;
    entry["varName"] = varName;
    entry["varValue"] = varValue;
    entry["watchdogCode"] = isWatchdogReset ? "123" : "";
    entry["watchdogText"] = isWatchdogReset ? MYDBG_getWatchdogText() : "";

    file = LittleFS.open("/mydbg_data.json", "w");
    if (file)
    {
        serializeJson(doc, file);
        file.close();
    }

    // Wenn Watchdog aktiv: zusätzlich auch in die Watchdog-Datei speichern
    if (isWatchdogReset)
    {
        MYDBG_logToWatchdog(entry);
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
}

// Web-Debug-Seite starten
void MYDBG_startWebDebug()
{
    // Hauptseite (HTML-Seite anzeigen)
    MYDBG_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                    { request->send(200, "text/html", MYDBG_HTML_PAGE); });

    // Logdatei ausgeben (richtiger Pfad: /mydbg_data.json)
    MYDBG_server.on("/mydbg_data.json", HTTP_GET, [](AsyncWebServerRequest *request)
                    {
        if (!LittleFS.exists("/mydbg_data.json")) {
            request->send(404, "text/plain", "Logdatei nicht gefunden");
            return;
        }
        File file = LittleFS.open("/mydbg_data.json", "r");
        request->send(file, "/mydbg_data.json", "application/json", false);
        file.close(); });

    // Watchdogdatei ausgeben (richtiger Pfad: /mydbg_watchdog.json)
    MYDBG_server.on("/mydbg_watchdog.json", HTTP_GET, [](AsyncWebServerRequest *request)
                    {
        if (!LittleFS.exists("/mydbg_watchdog.json")) {
            request->send(404, "text/plain", "Watchdogdatei nicht gefunden");
            return;
        }
        File file = LittleFS.open("/mydbg_watchdog.json", "r");
        request->send(file, "/mydbg_watchdog.json", "application/json", false);
        file.close(); });

    // Logs löschen
    MYDBG_server.on("/deleteLogs", HTTP_GET, [](AsyncWebServerRequest *request)
                    {
        deleteJsonLogs();
        request->send(200, "text/plain", "Logs gelöscht"); });

    // Protokoll AN
    MYDBG_server.on("/enableProtocol", HTTP_GET, [](AsyncWebServerRequest *request)
                    {
        protokollAktiv = true;
        request->send(200, "text/plain", "Protokoll AN"); });

    // Protokoll AUS
    MYDBG_server.on("/disableProtocol", HTTP_GET, [](AsyncWebServerRequest *request)
                    {
        protokollAktiv = false;
        request->send(200, "text/plain", "Protokoll AUS"); });

    // Protokoll-Status (für Status-Anzeige beim Seitenladen)
    MYDBG_server.on("/getProtocolState", HTTP_GET, [](AsyncWebServerRequest *request)
                    {
        String state = protokollAktiv ? "AN" : "AUS";
        request->send(200, "text/plain", state); });

    // WebSocket starten
    MYDBG_server.addHandler(&MYDBG_ws);

    // Server starten
    MYDBG_server.begin();
}

// Zeitstempel (lokal oder [keine Zeit])
inline String MYDBG_getTimestamp()
{
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("[MYDBG] Achtung: Zeit nicht synchronisiert!");
        return "[keine Zeit]";
    }
    char buf[30];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(buf);
}

// Zeitsynchronisation starten
inline void MYDBG_initTime(const char *ntpServer)
{
    configTime(0, 0, ntpServer);
    Serial.println("NTP-Zeit initialisiert");
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
}


void processSerialInput()
{
    unsigned long timeout = MYDBG_menuFirstCall ? 15000 : MYDBG_menuTimeout;
    MYDBG_menuFirstCall = false;

    Serial.printf("> Eingabe (%lus Timeout) Menue # + CRLF: ", timeout / 1000);
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
        Serial.println("[MYDBG] Kein Eintrag – Timeout abgelaufen.");
        return;
    }

    Serial.println("Du hast eingegeben: " + input);

    // Eingabe verarbeiten
    if (input == "1")
    {
        MYDBG_isEnabled = true;
        MYDBG_stopEnabled = true;
        MYDBG_setWatchdog(MYDBG_WDT_EXTENDED);
        Serial.println("[MYDBG] Modus 1 gesetzt: Debug AUSGABE + STOP aktiv");
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
        Serial.println("[MYDBG] Modus 3 gesetzt: Debug AUSGABE + STOP AUS");
    }
    else if (input == "4")
    {
        MYDBG_isEnabled = true;
        MYDBG_stopEnabled = true;
        MYDBG_setWatchdog(MYDBG_WDT_EXTENDED);
        MYDBG_webDebugEnabled = true;
        MYDBG_startWebDebug();

        Serial.println("\n[MYDBG] Modus 4 Web-Debug aktiv: http://" + WiFi.localIP().toString() + "/");
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
}

// Konsolenmenü für Debug-Einstellungen
inline void MYDBG_MENUE_IMPL(const char *aufruferFunc)
{
    if (!MYDBG_filesystemReady)
        MYDBG_initFilesystem();
   while (Serial.available())
       Serial.read(); // Eingabepuffer leeren
   
    Serial.println();
    Serial.println(String("=== MYDBG Menü (aufgerufen in: ") + aufruferFunc + ") ===");
    Serial.println(MYDBG_isEnabled && MYDBG_stopEnabled ? "x 1 = Debug AUSGABE + STOP aktiv" : "  1 = Debug AUSGABE + STOP aktiv");
    Serial.println(MYDBG_isEnabled && !MYDBG_stopEnabled ? "x 2 = Nur Debug AUSGABE aktiv" : "  2 = Nur Debug AUSGABE aktiv");
    Serial.println(!MYDBG_isEnabled && !MYDBG_stopEnabled ? "x 3 = Debug AUSGABE + STOP AUS" : "  3 = Debug AUSGABE + STOP AUS");
    Serial.println(MYDBG_webDebugEnabled ? "x 4 = Web-Debug anzeigen (status.html)" : "  4 = Web-Debug anzeigen (status.html)");
    Serial.println(MYDBG_webDebugEnabled ? "  5 = Web-Debug beenden" : "x 5 = Web-Debug beenden");
    Serial.println("  6 = JSON-Logs anzeigen (Serial-Ausgabe)");
    Serial.println("  7 = Alle JSON-Logs löschen (Filesystem)");
    processSerialInput(); // Eingabe verarbeiten
}
        
#define MYDBG_MENUE() MYDBG_MENUE_IMPL(__FUNCTION__)

#endif // MYDBG_H
