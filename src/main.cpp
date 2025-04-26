// Datei: main.cpp – Beispiel für MYDBG Good-Case-Demo
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "MYDBG.h" // Debug-Headerdatei
#include "secrets.h" // enthält meine WLAN_SSID und WIFI_PASS

// secrets.h enthält die WLAN-Zugangsdaten, wenn keine vorhanden ist  :
//const char *WIFI_SSID = "WLAN SSID";             // "WLAN SSID" eingeben
//const char *WIFI_PASS = "WLAN Passwort";         // "WLAN Passwort" eingeben

int zyklus = 0;
bool loopEnde = false;

void connectToWiFiMitTimeout(const char *ssid, const char *passwort, int timeoutSek = 15)
{
  Serial.printf("Verbinde mit SSID: %s\n", ssid);
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
    Serial.println("\nVerbindung fehlgeschlagen.");
    MYDBG(9, "WLAN fehlgeschlagen" WIFI_SSID);  //mit maximaler Wartezeit von 9 Sekunden
  }
}

void good_Anzeige_Serial()
{
  float temperatur = 23.7; // Beispielwert für Temperatur
  
  Serial.println();
  Serial.println("=== Beispiel für normale Serial.println Ausgaben mit delay 2.Sek zum lesen===");
  Serial.print("Temperatur: ");
  Serial.print(temperatur);
  Serial.println(" °C");
  Serial.println();
  delay(2000); // 2 Sekunden warten

  Serial.println("=== Beispiel für MYDBG-Serial-Ausgaben mit Wartezeit 9Sek. zum lesen ===");
  Serial.println("Beschreibung: Stop für 9Sek. Ausgabe:[MYDBG] > Zeilennummer | aufrufende Funktion() | Datum Uhrzeit | Laufzeit in millis | Text | Variable | Wert");
  MYDBG(9, "9 Sek. Anzeige: Temperaturwert anzeigen in C° ", temperatur); 
    
}
// good = Gute Funktion (stürtzt nicht ab ) zur Funktionsdarstellung von MYBDG
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
  good_Schritt1();
  good_Schritt2();
  good_Schritt3();
  loopEnde = false; // wenn hier neu auf "false" gesetzt wird, läuft Schleife 1,2,3 weiter. Ändern in "true" um Schleife zu stoppen
  if (loopEnde)
    return;
}

void setup()
{
  Serial.begin(115200);
  delay(200);
  
  connectToWiFiMitTimeout(WIFI_SSID, WIFI_PASS); // WLAN verbinden wenn vorhanden

  MYDBG_initTime(); // NTP-Zeit initialisieren wird im Main aufgefrufen wenn WLAN zur Verfügung steht
  MYDBG_MENUE(); // Konsolenmenü für Debug-Einstellungen

  good_Anzeige_Serial();
}

void loop()
{
  MYDBG_MENUE();

  good_GesamtSchleife(); // einmalige Schleife als Beispiel für die Verwendung von MYDBG
  
}
