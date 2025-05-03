# MYDBG – Grundeinstellungen und Initialverhalten

**Stand: 04.2025**

---

## Allgemeine Beschreibung

Die Bibliothek `MYDBG.h` dient der strukturierten Debug-Ausgabe über serielle Schnittstelle und WebSocket. Sie stellt HTML-Statusseiten, Watchdog-Überwachung, Dateilogs über LittleFS und ein interaktives Konsolenmenü zur Verfügung.

---

## Grundeinstellungen (Konstanten und globale Flags)

### Konstante Limits

* `MYDBG_MAX_LOGFILES = 10`  – Anzahl gespeicherter Logeinträge (FIFO-Verfahren)
* `MYDBG_MAX_WATCHDOGS = 10` – Anzahl gespeicherter Watchdog-Einträge
* `MYDBG_WDT_DEFAULT = 10`    – Standard-Timeout für den Watchdog (Sekunden)
* `MYDBG_WDT_EXTENDED = 300`  – Erweiterter Timeout für Tests (Sekunden)

### Kompilieroptionen (per `#define`)

* `MYDBG_NO_AUTOINIT`  – Unterdrückt automatische Initialisierung (Filesystem, Zeit, Webserver)
* `MYDBG_WEBDEBUG_NUR_MANUELL` – Web-Debug/Webserver muss manuell gestartet werden

---

## Globale Steuerflags zur Laufzeit

| Variable                | Funktion                                                   |
| ----------------------- | ---------------------------------------------------------- |
| `MYDBG_timeInitDone`    | Zeit (NTP) wurde erfolgreich synchronisiert                |
| `MYDBG_warnedAboutTime` | Warnung wegen fehlender Zeit wurde bereits ausgegeben      |
| `MYDBG_isEnabled`       | Debug-Ausgabe ist aktiviert                                |
| `MYDBG_stopEnabled`     | Wait-Funktion in `MYDBG(...)` erlaubt                      |
| `MYDBG_webDebugEnabled` | Web-Debug (WebSocket + HTML-Seite) aktiv                   |
| `MYDBG_webClientActive` | WebSocket-Client ist aktuell verbunden                     |
| `MYDBG_filesystemReady` | LittleFS wurde erfolgreich initialisiert                   |
| `MYDBG_menuFirstCall`   | Erstaufruf des Konsolenmenü                                |
| `MYDBG_menuTimeout`     | Timeout für serielle Eingabe in Millisekunden (z. B. 5000) |

---

## Webserver-Integration

* Port: `80`
* WebSocket: Pfad `/ws`
* Debug-Webseite: `/status.html`
* JSON-Logdateien:

  * `/mydbg_data.json`         – Letzte Debug-Logs
  * `/mydbg_watchdog.json`     – Watchdog-Auslösungen
  * `/mydbg_status.json`       – Letzter Status

---

## WebSocket-Kommandos

* `PROTOKOLL_EIN` – Aktiviert serielle/Web-Ausgabe
* `PROTOKOLL_AUS` – Deaktiviert Ausgaben komplett

---

## Konsolenmenü (MYDBG\_MENUE)

Menü wird per `MYDBG_MENUE()` aufgerufen und erlaubt folgende Eingaben:

| Taste | Bedeutung                           |
| ----- | ----------------------------------- |
| `1`   | Debug AUSGABE + wait aktiv          |
| `2`   | Nur Debug AUSGABE aktiv             |
| `3`   | Debug AUSGABE + wait AUS            |
| `4`   | Web-Debug aktivieren                |
| `5`   | Web-Debug deaktivieren              |
| `6`   | JSON-Logs anzeigen (Serial-Ausgabe) |
| `7`   | Alle JSON-Logs löschen              |

**Timeout**: Standard 5 Sekunden (anpassbar über `MYDBG_menuTimeout`)

---

## Automatische Initialisierung (Makro-intern)

Wenn **nicht** durch `MYDBG_NO_AUTOINIT` deaktiviert, wird durch jedes Makro `MYDBG(...)` automatisch Folgendes ausgeführt:

* **Filesystem starten** über `MYDBG_initFilesystem()`
* **Zeitsynchronisation** via `MYDBG_initTime("pool.ntp.org")`
* **Web-Debug aktivieren**, wenn nicht manuell deaktiviert

---

## Beispiel für Makro-Aufruf

```cpp
float RohrTemp = 88.5;
MYDBG(3, "Rohrtemperatur zu hoch", RohrTemp);
```

Diese Zeile erzeugt:

* serielle Ausgabe
* JSON-Logeintrag
* WebSocket-Nachricht
* optional Pause (3 Sekunden \* 1000ms), wenn `stopEnabled = true`

---
