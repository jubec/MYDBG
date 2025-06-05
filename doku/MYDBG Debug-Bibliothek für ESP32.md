# 🧩 MYDBG Debug-Bibliothek für ESP32

## 📄 Überblick

In der typischen Arduino-Entwicklung dienen `Serial.print()` und `delay()` oft als einfache Hilfsmittel, um den Programmablauf und den Wert einzelner Variablen über die serielle Konsole zu beobachten. Doch diese Methode hat viele Grenzen:

- Man sieht nicht, in welcher Funktion, an welcher Zeile oder zu welchem Zeitpunkt der Aufruf stattgefunden hat.
- Es fehlt jegliche Dokumentation über zurückliegende Ereignisse.
- Eine Überwachung ist nur per USB-Kabel möglich.

Genau aus diesem Bedarf heraus ist **MYDBG** entstanden: Ein modernes Debug-Werkzeug, das die typischen Grenzen von `Serial.print()` überwindet.

### MYDBG ermöglicht:

- Präzise Ortung von Ereignissen (Funktion, Zeilennummer)
- Zeitliche Einordnung mit Zeitstempel und `millis()`
- Logging in JSON-Dateien (LittleFS im Flashspeicher)
- Live-Web-Debugging über WLAN & WebSocket, auch ohne USB

Mit **MYDBG** kannst du den Programmablauf deines ESP32 nachvollziehen, als ob du direkt dabei wärst – selbst wenn das Gerät in einem Gehäuse steckt oder überhaupt nicht an USB angeschlossen ist.

---

## 🧪 Erste Schritte & Integration (VS Code & Arduino IDE)

### 🔧 Visual Studio Code mit PlatformIO

**Dateien kopieren:**

- `MYDBG.h` nach `/include/`
- Beispielprogramm `main.cpp` in `/src/`
- `secrets.h` erstellen und in `/include/` speichern (enthält `WIFI_SSID` und `WIFI_PASS`)
- Alternativ: `WIFI_SSID` und `WIFI_PASS` direkt in `main.cpp` eintragen

**Includes ergänzen:**

```cpp
#include "MYDBG.h"
#include "secrets.h"
```

**Funktionen aufrufen:**

- WLAN-Verbindung z. B. im `setup()` über `connectToWiFiMitTimeout(WIFI_SSID, WIFI_PASS);`
- Debugausgaben über `MYDBG()` einbauen
- Optional: Menü im `loop()` aufrufen mit `MYDBG_MENUE();`

**Webseite testen:**

Nach erfolgreicher WLAN-Verbindung:  
Browser öffnen → `http://[ESP32-IP]/status.html`  
Logdateien können per Webinterface angezeigt oder gelöscht werden.

---

## 🔍 Verhalten bei Initialisierung

MYDBG initialisiert automatisch das Dateisystem, die Zeit (via NTP) und den Webserver – jedoch **nur beim ersten `MYDBG()`-Aufruf mit `wait > 0`**.  
Nur im Fehlerfall erfolgt eine Ausgabe auf der Konsole (z. B. wenn die Zeit nicht verfügbar ist oder das Dateisystem nicht bereit ist).

---

## ⚠️ Hinweis zur Systemlast

Die Systemlast des ESP32 durch MYDBG ist erhöht – vor allem durch Lesepausen (`wait`) und das Speichern der Debug-Daten im Flash. Der Watchdog wird daher auf einen höheren Wert gesetzt.

Da Debugging den Programmablauf beeinflussen kann, empfiehlt es sich, vor dem Echtbetrieb MYDBG vollständig zu deaktivieren:

```cpp
// #include "MYDBG.h"
// alle MYDBG(...) Zeilen per Suchen & Ersetzen durch //MYDBG(...)
```

So lässt sich Debugging bei Bedarf auch schnell wieder aktivieren.

---

## 💡 Beispielstart in `main.cpp` (gekürzt)

```cpp
void setup() {
  Serial.begin(115200);
  connectToWiFiMitTimeout(WIFI_SSID, WIFI_PASS);
  MYDBG(2, "Starte Debug-Modus", millis());
}

void loop() {
  MYDBG_MENUE(); // einmalig oder zyklisch aufrufen
  // ... weitere MYDBG-Funktionen testen
}
```

---

## 🧰 Arduino IDE (alternativ)

- `MYDBG.h` im gleichen Ordner wie die `.ino`-Datei ablegen
- `secrets.h` ebenfalls dort anlegen (mit SSID und Passwort)

Am Anfang der `.ino`-Datei:

```cpp
#include "MYDBG.h"
#include "secrets.h"
```

Verwendung wie oben beschrieben.

---

## ⚙️ Hauptfunktionen

| Funktion | Beschreibung |
|----------|--------------|
| `MYDBG(0, "Nachricht")` | Serielle Ausgabe ohne Pause, Logging oder WebSocket. Ideal für schnelle Tests. |
| `MYDBG(0, "Nachricht", variable)` | Wie oben, zusätzlich mit Variablennamen und Wert. |
| `MYDBG(wait, "Nachricht") (1..9)` | Ausgabe mit Zeitstempel, Pause, JSON-Logging, WebSocket. |
| `MYDBG(wait, "Nachricht", variable)` | Wie oben, mit Variablenausgabe. Vollständige Logausgabe. |
| `MYDBG_MENUE()` | Interaktives Konsolenmenü. |
| `MYDBG_initFilesystem()` | Initialisiert LittleFS, erzeugt/verwaltet Debug-Dateien. |
| `MYDBG_startWebDebug()` | Startet Debug-Webseite & WebSocket unter `/status.html`. |
| `MYDBG_getTimestamp()` | Gibt aktuellen Zeitstempel zurück. |
| `MYDBG_setWatchdog(sek)` | Setzt Watchdog mit definierter Zeit. |
| `displayJsonLogs()` | Gibt gespeicherte Logs auf Serial aus. |
| `deleteJsonLogs()` | Löscht gespeicherte Log-Dateien. |
| `wlanVerbindungPruefenAlleXMin(min)` | Zyklische WLAN-Verbindungsprüfung. |
| `connectToWiFiMitTimeout(ssid, pw, timeout)` | WLAN-Verbindung mit Timeout herstellen. |

---

## 🌐 Weboberfläche

**Aufrufbar unter:** `http://[ESP-IP]/status.html`

### Funktionen:

- Protokoll aktivieren/deaktivieren
- Logs anzeigen/herunterladen:
  - `/mydbg_data.json`
  - `/mydbg_watchdog.json`
  - `/mydbg_status.json`
- Logs löschen via `/delete_logs`
- Live-Daten via WebSocket
- Zusatzinfos: Speicher, Reset-Grund, `millis()`

---

## ⏳ Wartemodi (`waitIndex` in `MYDBG()`)

| Wert | Bedeutung |
|------|-----------|
| 0 | Nur Konsolenausgabe. Keine Pause, kein Logging/WebSocket. |
| 1..9 | Ausgabe + Pause (Sekunden), Logging + WebSocket. |
| >9 | Wird automatisch auf 9 begrenzt. |

---

## 📋 Konsolenmenü zur Laufzeit

⚠️ Stelle sicher, dass dein serielles Terminal auf **CR+LF** eingestellt ist.

| Eingabe | Funktion |
|---------|----------|
| 1 | Debug + Wait aktiv, Eintrag in Logs |
| 2 | Nur Debug-Ausgabe (schneller) |
| 3 | Debug vollständig deaktivieren |
| 4 | Web-Debug aktivieren |
| 5 | Web-Debug deaktivieren |
| 6 | Logs anzeigen (Serial) |
| 7 | Logs löschen |

---

## 💡 Beispiel

```cpp
int sensorwert = 42;
MYDBG(0, "Sensorwert erfasst", sensorwert); // Nur Konsole
MYDBG(2, "Sensorwert erfasst", sensorwert); // 2s Pause, Logging, Web
```

---

## 🛠️ Weitere Ideen

- Konfiguration in NVS speichern
- Debug-Ausgabe gezielt an-/abschalten

---

## ☕ Zum Schluss

**Wer Debuggen kann, kann auch Programmieren –  
und wer beides kann, fragt dann ChatGPT. 😊**