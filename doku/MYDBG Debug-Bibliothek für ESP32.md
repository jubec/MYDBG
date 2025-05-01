## 🧩 MYDBG Debug-Bibliothek für ESP32

### 📄 Überblick
In der typischen Arduino-Entwicklung dienen `Serial.print()` und `delay()` oft als einfache Hilfsmittel, um den Programmablauf und den Wert einzelner Variablen über die serielle Konsole zu beobachten. Doch diese Methode hat viele Grenzen:

- Man sieht **nicht**, in **welcher Funktion**, an **welcher Zeile** oder zu **welchem Zeitpunkt** der Aufruf stattgefunden hat.
- Es fehlt jegliche Dokumentation über zurückliegende Ereignisse.
- Eine Überwachung ist **nur über USB-Kabel** möglich.

Genau aus diesem Bedarf heraus ist `MYDBG` entstanden:  
Ein modernes Debug-Werkzeug, das die typischen Grenzen von `Serial.print()` überwindet.  

**MYDBG ermöglicht:**
- Präzise **Ortung** von Ereignissen (Funktion, Zeilennummer)
- **Zeitliche Einordnung** mit Zeitstempel und `millis()`
- **Logging** in JSON-Dateien im internen Flash (LittleFS)
- **Live-Web-Debugging** über WLAN & WebSocket, auch ohne USB

Mit `MYDBG` kannst du den Programmablauf deines ESP32 nachvollziehen, als ob du direkt dabei wärst – selbst wenn das Gerät in einem Gehäuse steckt oder überhaupt nicht an USB angeschlossen ist.

---

### 🧪 Erste Schritte & Integration (VS Code & Arduino IDE)

#### 🔧 Visual Studio Code mit PlatformIO
1. **Dateien kopieren:**
   - `MYDBG.h` nach `/include/`
   - Beispielprogramm (`main.cpp`) in `/src/`
   - `secrets.h` erstellen und in `/include/` speichern (enthält `WIFI_SSID` und `WIFI_PASS`).

2. **Includes ergänzen:**
   ```cpp
   #include "MYDBG.h"
   #include "secrets.h"
   ```

3. **Funktionen aufrufen:**
   - WLAN-Verbindung z. B. im `setup()` über `connectToWiFiMitTimeout(WIFI_SSID, WIFI_PASS);`
   - Debugausgaben über `MYDBG()` einbauen.
   - Optional: Menü im `loop()` aufrufen mit `MYDBG_MENUE();`

4. **Webseite testen:**
   - Nach erfolgreicher WLAN-Verbindung: Browser öffnen → `http://[ESP32-IP]/status.html`
   - Logdateien können per Webinterface gelöscht oder angezeigt werden.

#### 💡 Beispielstart in `main.cpp` (gekürzt):
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

#### 🧰 Arduino IDE (alternativ)
- `MYDBG.h` im gleichen Ordner wie die `.ino`-Datei ablegen
- `secrets.h` ebenfalls dort anlegen (mit SSID und Passwort)
- Am Anfang der `.ino`-Datei:
  ```cpp
  #include "MYDBG.h"
  #include "secrets.h"
  ```
- In `setup()` und `loop()` wie oben verwenden

---

### ⚙️ Hauptfunktionen
| Funktion | Beschreibung |
|----------|--------------|
| `MYDBG(0, "Nachricht")` | Gibt einfache Debug-Ausgabe auf der seriellen Konsole aus – ohne Pause, ohne Logging oder Webübertragung. Ideal für schnelle Infos während der Entwicklung. |
| `MYDBG(0, "Nachricht", variable)` | Wie oben, zeigt zusätzlich den Variablennamen und -wert. Wird **nicht** geloggt oder per WLAN übertragen. |
| `MYDBG(wait, "Nachricht")` *(mit wait = 1..9)* | Gibt Debug-Informationen mit Zeitstempel aus, pausiert `wait` Sekunden, schreibt in JSON-Log und überträgt Live per WebSocket. |
| `MYDBG(wait, "Nachricht", variable)` | Wie oben, mit zusätzlicher Anzeige einer Variable. Diese Aufrufe erzeugen die **vollständige Logausgabe** (JSON + Web). |
| `MYDBG_MENUE()` | Interaktives Konsolenmenü zur Laufzeitsteuerung |
| `MYDBG_initFilesystem()` | Initialisiert das LittleFS-Dateisystem. Stellt die Debug-Logs unter `/mydbg_data.json`, `/mydbg_watchdog.json` und `/mydbg_status.json` per Webzugriff zur Verfügung. |
| `MYDBG_startWebDebug()` | Startet Web-Debug-Webseite & WebSocket unter `/status.html` |
| `MYDBG_getTimestamp()` | Liefert formatierten Zeitstempel (lokal oder Ersatztext) |
| `MYDBG_setWatchdog(sek)` | Initialisiert Software-Watchdog (Timeout in Sekunden) |
| `displayJsonLogs()` | Gibt gespeicherte Logs auf Serial aus |
| `deleteJsonLogs()` | Löscht alle gespeicherten Log-Dateien (data, status, watchdog) |
| `wlanVerbindungPruefenAlleXMin(min)` | Prüft zyklisch WLAN-Verbindung (eigene Erweiterung, derzeit optional und nicht aktiv). Nützlich, wenn z. B. ein temporäres Netzwerk zur Kommunikation aufgebaut wird. |
| `connectToWiFiMitTimeout(ssid, pw, timeout)` | Stellt WLAN-Verbindung mit Timeout her |

---

### 🌐 Weboberfläche
- Aufrufbar unter: `http://[ESP-IP]/status.html`
- Funktionen:
  - Protokoll aktivieren/deaktivieren
  - JSON- und Watchdog-Logs anzeigen (`/mydbg_data.json`, `/mydbg_watchdog.json`, `/mydbg_status.json`)
  - Logdateien löschen per Button `/delete_logs`
- Zeigt Live-Daten über WebSocket
- Zusätzliche Infos: Speicherplatz, Reset-Grund, millis()

---

### ⏳ Wartemodi (`waitIndex` im `MYDBG()`)
| Wert | Bedeutung |
|------|-----------|
| `0` | Nur Konsolenausgabe. Keine Pause, kein WebSocket, kein Logging. |
| `1..9` | Wartezeit in Sekunden. Ausgabe mit Zeitstempel, JSON-Logging, WebSocket-Übertragung. |
| `>9` | Wird automatisch auf 9s begrenzt |

---

### 📋 Konsolenmenü zur Laufzeit
Eingabe per seriellem Terminal (15s beim ersten Aufruf):

| Eingabe | Funktion |
|---------|----------|
| `1` | Debug-Ausgabe + Stop aktiv |
| `2` | Nur Debug-Ausgabe (keine Pause) |
| `3` | Debug deaktiviert |
| `4` | Web-Debug aktivieren |
| `5` | Web-Debug beenden |
| `6` | Logs anzeigen (Serial) |
| `7` | Logs löschen |

---

### 💡 Beispiel
```cpp
int sensorwert = 42;
MYDBG(0, "Sensorwert erfasst", sensorwert); // Nur Konsole, keine Pause, kein Logging
MYDBG(2, "Sensorwert erfasst", sensorwert); // 2 Sek. Pause, Logging und Web-Ausgabe
```

---

### 🛠️ Weitere Idee
- Konfiguration dauerhaft speichern (z. B. NVS)

