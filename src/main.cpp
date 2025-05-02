
// Datei: main.cpp – Beispiel für MYDBG 
// Beschreibung: Beispiel für die Verwendung der MYDBG-Bibliothek zur Ausgabe von Debug-Informationen und zur Überwachung des Programms.
// Diese Datei enthält die Hauptlogik des Programms, einschließlich WLAN-Verbindung, Debug-Ausgaben und  Beispielanwendungen.
// Die MYDBG-Bibliothek wird verwendet, um Debug-Informationen in der Konsole und über das Web bereitzustellen.
// Die Datei enthält auch Beispiele für gute und schlechte Programmierpraktiken, um die Verwendung der MYDBG-Bibliothek zu demonstrieren.

// Durch auskommentieren im loop werden bad-Funktionen nicht aufgerufen

#include <Arduino.h>
#include <WiFi.h> // für WLAN
#include <time.h> // für Zeitstempel
#include "MYDBG.h" // Debug-Headerdatei
#include "secrets.h" // enthält meine WLAN_SSID und WIFI_PASS

// secrets.h enthält die WLAN-Zugangsdaten, wenn keine secrets.h vorhanden ist  :
//const char *WIFI_SSID = "WLAN SSID";             // "WLAN SSID" eingeben
//const char *WIFI_PASS = "WLAN Passwort";         // "WLAN Passwort" eingeben

// Variable zur darstellung im Programm MYDBG
int zyklus = 0;
bool loopEnde = false;
int z = 10; // Globale Variable

void connectToWiFiMitTimeout(const char *ssid, const char *passwort, int timeoutSek = 15)
{
  Serial.printf("WLAN verbinde mit SSID: %s\n", ssid);
  WiFi.begin(ssid, passwort);
  unsigned long t0 = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - t0 < timeoutSek * 1000)
  {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nVerbunden. IP: " + WiFi.localIP().toString());
  }
  else
  {
    MYDBG(9, "WLAN fehlgeschlagen" WIFI_SSID);  //mit maximaler Wartezeit von 9 Sekunden
  }
}
// WLAN regelmäßig prüfen (alle X Minuten), und ggf. erneut verbinden
void wlanVerbindungPruefenAlleXMin(unsigned int minuten = 5)
{
  static unsigned long letztePruefung = 0;
  unsigned long intervall = minuten * 60 * 1000UL;
  unsigned long jetzt = millis();

  // absichern gegen Überlauf
  if (jetzt - letztePruefung >= intervall || jetzt < letztePruefung)
  {
    letztePruefung = jetzt;

    if (WiFi.status() != WL_CONNECTED)
    {
      MYDBG(1, "WLAN getrennt – versuche erneut zu verbinden");
      connectToWiFiMitTimeout(WIFI_SSID, WIFI_PASS);
    }
    else
    {
      MYDBG(0, "WLAN-Verbindung OK");
    }
  }
}

void good_Anzeige_Serial()
{
  float temperatur = 23.7; // Beispielwert für Temperatur
  
  // === Beispiel für "normale" Serial.println Ausgaben mit delay 2.Sek zum lesen ===
  Serial.println();
  Serial.println("=== Beispiel für normale Serial.println Ausgaben mit delay 2.Sek zum lesen===");
  Serial.print("Temperatur: ");
  Serial.print(temperatur);
  Serial.println(" °C");
  Serial.println();
  delay(2000); // 2 Sekunden warten
  
  // === Beispiel für MYDBG-Serial-Ausgaben mit Wartezeit 9Sek. zum Text lesen ===
  Serial.println("=== Beispiel für MYDBG-Serial-Ausgaben mit Wartezeit 9Sek. zum lesen ===");
  // === Kurzbeschreibung der ausgabe
  Serial.println("=== Beschreibung: Stop für 9Sek. Ausgabe:[MYDBG] > Zeilennummer | aufrufende Funktion() | Datum Uhrzeit | Laufzeit in millis | Text | Variable | Wert");
  
  //=== Ausgabe mit MYDBG
  MYDBG(9, "9 Sek. Anzeige: Temperaturwert anzeigen in C° ", temperatur); 
    
}
// === good_= Gute Funktion (stürtzt nicht ab ) zur Funktionsdarstellung von MYBDG
void good_Schritt1()
{
  MYDBG(0, "In Schritt 1 nur Textausgabe um z.B einen Hilfstext aus zugeben"); // 0 Sekunden warten, keine Json-Ausgabe
  // hier würde z.B. ProgrammCode stehen
}// Ende von good_Schritt1()

void good_Schritt2()
{
  int wert = 42;
  MYDBG(0, "In Schritt 2 Text mit Wert", wert); // 0 Sekunden warten, Variable , keine Json-Ausgabe
}// Ende von good_Schritt2()

void good_Schritt3()
{
  MYDBG(9, "In Schritt 3, alles angezeigen, 9 Sek.stoppen und speichern", zyklus); // 9 Sekunden warten, Json-Ausgabe im WEB

  zyklus++;
  if (zyklus >= 3)
  {
    loopEnde = true; 
    MYDBG(0, "Ende des Duchlaufs", zyklus);
  }
}//Ende von good_Schritt3()

void good_GesamtSchleife()
{
  int z = 20; // lokale Variable
  MYDBG(1,"Lokale Variable z", z); // 0 Sekunden warten, keine Json-Ausgabe
  
  good_Schritt1();
  good_Schritt2();
  good_Schritt3();
  loopEnde = false; // wenn hier neu auf "false" gesetzt wird, läuft Schleife 1,2,3 weiter. Ändern in "true" um Schleife zu stoppen
  if (loopEnde)
    return;
} // Ende von good_GesamtSchleife()


// === bad_ = Schlechte Funktion (stürtzt ab ) zur Funktionsdarstellung von MYBDG
void bad_Watchdog()
{
  // Vorher Watchdog kurz setzen (z. B. 5 Sekunden)
  MYDBG_setWatchdog(5);   
  Serial.println("[TEST] Watchdog auf 5 Sekunden gesetzt"); 
  // Letzte Nachricht vor dem Absturz, wird vollständig in JSON-Log übernommen
  MYDBG(9, "Schlechte Funktion: Endlosschleife gestartet – Watchdog wird zuschlagen", zyklus);

  // Jetzt die Endlosschleife ohne Watchdog-Reset
  while (true)
  {
    volatile int x = millis(); // blockierende Schleife
  }
}// Ende von bad_Watchdog()

void setup()
{
  Serial.begin(115200);
  delay(200);
  
  connectToWiFiMitTimeout(WIFI_SSID, WIFI_PASS); // WLAN verbinden wenn vorhanden
  
 // MYDBG_MENUE(); // Konsolenmenü für Debug-Einstellungen

  good_Anzeige_Serial();
  
}// Ende von setup()

void loop()
{
  wlanVerbindungPruefenAlleXMin(2); // z. B. alle 2 Minuten
  Serial.printf("[MYDBG] MAX_LOGFILES = %d\n", MYDBG_MAX_LOGFILES);
  MYDBG_MENUE(); // Konsolenmenü für Debug-Einstellungen

 
  if (!loopEnde) // Schleife nur solange durchlaufen, wie loopEnde = false ist
  {
    good_GesamtSchleife();
  }
  
  bad_Watchdog(); // Watchdog-Provo, wenn diese Funktion aufgerufen wird, stürzt das Programm ab und der Watchdog wird ausgelöst

}// Ende von loop()
