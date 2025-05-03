# MYDBG – Grundeinstellungen und Initialverhalten

**Stand: 04.2025**

---

## Allgemeine Beschreibung

Die Bibliothek `MYDBG.h` dient der strukturierten Debug-Ausgabe über serielle Schnittstelle und WebSocket. Sie stellt HTML-Statusseiten, Watchdog-Überwachung, Dateilogs über LittleFS und ein interaktives Konsolenmenü zur Verfügung.

### Warum MYDBG sinnvoll ist

Viele beginnen beim Debugging klassisch mit `Serial.print()` und `delay()`. Das reicht für einfache Tests, ist aber schnell unübersichtlich:

* Welche Zeile hat den Fehler ausgelöst?
* Welche Variable hatte welchen Wert?
* Wann ist etwas passiert?
* Wo befand sich das Programm im Ablauf?

`MYDBG` liefert genau diese Informationen:

* Zeilennummer und Funktionsname
* Datum und Uhrzeit (wenn verfügbar)
* Millis-Zeitstempel (auch ohne Uhr)
* Aussagekräftige Nachricht
* Name und Wert einer Variable

Die Ausgaben erfolgen gleichzeitig:

* **auf der seriellen Konsole**
* **in der HTML-Webseite (`status.html`)**
* **als JSON-Datei auf dem ESP-Dateisystem (LittleFS)**

Das alles unterstützt dich dabei, komplexere Abläufe und Fehler zu verstehen – **auch ohne angeschlossene serielle Konsole**.

Selbst wenn dein ESP „allein lebt“, kannst du per Webbrowser die gespeicherten JSON-Logs abrufen oder prüfen, ob ein Watchdog-Reset aufgetreten ist. Das Watchdog-Ereignis wird mit dem letzten gültigen `MYDBG(...)`-Eintrag angezeigt – kann aber zeitlich später erfolgt sein. Zusätzliche gezielte `MYDBG(...)`-Einträge helfen bei der Analyse des Ablaufes bis zum Absturz.

Die optionale Wartezeit (`MYDBG(3, ...)`) hilft beim schrittweisen Verstehen – kann aber auch deaktiviert werden, um den Ablauf flüssig durchlaufen zu lassen.

---

## Beispielanwendung mit Watchdog-Fehler

In der `main.cpp` kannst du eine kleine Anwendung bauen, die typische Debug-Ausgaben erzeugt und gezielt einen Watchdog-Fehler simuliert:

```cpp
void loop() {
    static int zaehler = 0;
    MYDBG(1, "Schleife gestartet", zaehler);

    if (zaehler == 3) {
        MYDBG(5, "Endlosschleife simulieren", zaehler);
        while (true) {
            // absichtlich kein delay und kein yield – Watchdog wird zuschlagen
        }
    }

    delay(1000);
    zaehler++;
}
```

Diese kleine Testschleife:

* gibt den Zählerstand aus
* simuliert nach einigen Schleifen eine Endlosschleife
* der Watchdog wird auslösen
* das Ereignis wird als JSON-Eintrag gespeichert

---

## Tipps zur Integration in bestehende Projekte

Während der Entwicklung kannst du `MYDBG(...)`-Befehle großzügig einsetzen. Doch im späteren Echtbetrieb solltest du gezielt nur an sinnvollen Stellen Debug-Ausgaben aktiv lassen:

* Nutze `MYDBG(...)` bevorzugt an **Start- und Endpunkten von Funktionen**, um Aufrufe und Rückgaben zu beobachten.
* **Zu viele Debug-Ausgaben stören** den Ablauf und belasten Flash und RAM. Daher solltest du ungenutzte Debug-Zeilen **auskommentieren**.

### Praktischer Tipp:

Benutze Suchen und Ersetzen, um Debug-Befehle temporär zu deaktivieren:

```cpp
MYDBG( → //MYDBG(
```

So kannst du sie jederzeit reaktivieren, ohne Code zu löschen.

### Lebenszeichen für WLAN-Betrieb

Ein `MYDBG(1, "Loop läuft")` im Hauptloop kann per WebSocket und JSON-Dateien zeigen, dass der ESP aktiv ist. Damit hast du eine Art „Pulsanzeige“ des Systems – sichtbar im Browser und speicherbar im Dateisystem.

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
