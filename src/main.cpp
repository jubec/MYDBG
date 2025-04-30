
// Datei: main.cpp – Beispiel für MYDBG 
// Beschreibung: Beispiel für die Verwendung der MYDBG-Bibliothek zur Ausgabe von Debug-Informationen und zur Überwachung des Programms.
// Diese Datei enthält die Hauptlogik des Programms, einschließlich WLAN-Verbindung, Debug-Ausgaben und  Beispielanwendungen.
// Die MYDBG-Bibliothek wird verwendet, um Debug-Informationen in der Konsole und über das Web bereitzustellen.
// Die Datei enthält auch Beispiele für gute und schlechte Programmierpraktiken, um die Verwendung der MYDBG-Bibliothek zu demonstrieren.

// Durch auskommentieren werdn bad-Funktionen nicht aufgerufen

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
}

void good_Schritt2()
{
  int wert = 42;
  MYDBG(0, "In Schritt 2 Text mit Wert", wert); // 0 Sekunden warten, Variable , keine Json-Ausgabe
}

void good_Schritt3()
{
  MYDBG(9, "In Schritt 3, alles angezeigen, 9 Sek.stoppen und speichern", zyklus); // 9 Sekunden warten, Json-Ausgabe im WEB

  zyklus++;
  if (zyklus >= 3)
  {
    loopEnde = true; 
    MYDBG(0, "Ende des Duchlaufs", zyklus);
  }
}

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
}


// === bad_ = Schlechte Funktion (stürtzt ab ) zur Funktionsdarstellung von MYBDG
void bad_Watchdog()
{
  MYDBG(0, "Schlechte Funktion: Starte Endlosschleife zur Watchdog-Provo");
  while (true)
  {
    // Kein delay, kein yield → blockiert CPU komplett
    // Der Watchdog (TWDT) sollte nach ca. 5 Sekunden zuschlagen
    volatile int x = millis(); // Dummy, damit Compiler nichts wegoptimiert
  }
}

void setup()
{
  Serial.begin(115200);
  delay(200);
  
  connectToWiFiMitTimeout(WIFI_SSID, WIFI_PASS); // WLAN verbinden wenn vorhanden
  
 // MYDBG_MENUE(); // Konsolenmenü für Debug-Einstellungen

  good_Anzeige_Serial();
  
}

void loop()
{
 // MYDBG_MENUE(); // Konsolenmenü für Debug-Einstellungen

 
  if (!loopEnde) // Schleife nur solange durchlaufen, wie loopEnde = false ist
  {
    good_GesamtSchleife();
  }
 
  
 
 // bad_Watchdog(); // Watchdog-Provo, wenn diese Funktion aufgerufen wird, stürzt das Programm ab und der Watchdog wird ausgelöst
}
