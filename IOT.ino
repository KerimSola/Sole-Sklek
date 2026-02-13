#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <NewPing.h>

#define WIFI_SSID "Sole"
#define WIFI_PASSWORD "solasola"

// Firebase URL-ovi
#define URL_START   "https://sole-sklek-iot-default-rtdb.europe-west1.firebasedatabase.app/komandaStart.json"
#define URL_MEASURE "https://sole-sklek-iot-default-rtdb.europe-west1.firebasedatabase.app/komandaMeasure.json"
#define URL_GORNJA  "https://sole-sklek-iot-default-rtdb.europe-west1.firebasedatabase.app/GornjaGranica.json"
#define URL_BROJ    "https://sole-sklek-iot-default-rtdb.europe-west1.firebasedatabase.app/BrojSklekova.json"

// Ultrazvuk
#define TRIGGER_PIN 14
#define ECHO_PIN 12
#define MAX_DISTANCE 70
#define DONJA_GRANICA 10

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
BearSSL::WiFiClientSecure client;

// Firebase optimizacija
unsigned long lastFirebaseCheck = 0;
int fb_start = 0;
int fb_measure = 0;

// Logika sklekova
int gornjaGranica = 0;
bool bioDole = false;
int brojSklekova = 0;

// Kontrola mjerenja
const unsigned long pingInterval = 200; // ms između mjerenja
unsigned long lastPingTime = 0;

int ZELENA = 4;
int CRVENA = 13;
int ZELENA_DOLE=5;
int ZELENA_GORE=16;

 
void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Povezivanje");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nPovezano!");
  client.setInsecure();

  pinMode(ZELENA, OUTPUT);
  pinMode(CRVENA, OUTPUT);
  pinMode(ZELENA_DOLE, OUTPUT);
  pinMode(ZELENA_GORE, OUTPUT);


}

void loop() {
  if (WiFi.status() != WL_CONNECTED) return;

  unsigned long currentMillis = millis();

  // === Firebase čitanje (svakih 0,2 sekunde) ======================
  if (currentMillis - lastFirebaseCheck > 200) {
    lastFirebaseCheck = currentMillis;
    fb_start   = getFirebaseInt(URL_START);
    fb_measure = getFirebaseInt(URL_MEASURE);
  }

  // === 0) MJERENJE GORNJE GRANICE ============================
  if (fb_measure == 1) {
    Serial.println("Mjerim gornju granicu...");
    int cm = sonar.ping_median(3) / US_ROUNDTRIP_CM;
    if (cm == 0) cm = 999;
    Serial.print("Gornja granica izmjerena: ");
    Serial.println(cm);

    sendNumber(URL_GORNJA, cm);
    sendNumber(URL_MEASURE, 0);
  }

  // === 1) NORMALNI RAD — mjerenje sklekova ==================
  if (fb_start == 1 && currentMillis - lastPingTime >= pingInterval) {
    lastPingTime = currentMillis;

     digitalWrite(ZELENA, HIGH); // upali LED
     digitalWrite(CRVENA, LOW); // ugasi LED
     

    // Povremeno ažuriraj gornju granicu sa Firebase-a (npr. svakih 5 sekundi)
    static unsigned long lastGornjaUpdate = 0;
    if (currentMillis - lastGornjaUpdate > 5000) {
      gornjaGranica = getFirebaseInt(URL_GORNJA);
      lastGornjaUpdate = currentMillis;
    }

    int gUp = gornjaGranica - 5;



    // BRZO mjerenje
    int cm = sonar.ping_cm();
    if (cm == 0) cm = 999;

    Serial.println(cm);

    // 1 – detekcija dole
    if (!bioDole && cm < DONJA_GRANICA) {
      bioDole = true;
      Serial.println("DETektovan DOLE!");
          digitalWrite(ZELENA_DOLE , HIGH); // Upali LED
    }

    // 2 – detekcija gore
    if (bioDole && cm > gUp && cm < gornjaGranica + 10) {
      brojSklekova++;
      bioDole = false;
      digitalWrite(ZELENA_GORE , HIGH); // Upali LED
      Serial.print("SKLEK BROJ: ");
      Serial.println(brojSklekova);
      sendNumber(URL_BROJ, brojSklekova);

      digitalWrite(ZELENA_DOLE , LOW); // Ugasi LED
      digitalWrite(ZELENA_GORE , LOW); // Ugasi LED
    }
  }
  else if (fb_start != 1) {
    // reset na STOP
    digitalWrite(ZELENA, LOW); // ugasi LED
    digitalWrite(CRVENA , HIGH); // Upali LED
    digitalWrite(ZELENA_DOLE , LOW); // Upali LED
    digitalWrite(ZELENA_GORE , LOW); // Upali LED

    bioDole = false;
    brojSklekova = 0;
    sendNumber(URL_BROJ, 0);
  }
}

// ================= Pomoćne funkcije ==========================
int getFirebaseInt(String url) {
  HTTPClient http;
  http.begin(client, url);
  int code = http.GET();
  if (code != 200) {
    http.end();
    return 0;
  }
  int value = http.getString().toInt();
  http.end();
  return value;
}

void sendNumber(String url, int num) {
  HTTPClient http;
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.PUT(String(num));
  http.end();
}
