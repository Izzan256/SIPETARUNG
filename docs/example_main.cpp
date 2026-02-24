#include <Arduino.h>
#include <MicrowaveRadar.h>

// Inisialisasi: Pin D2, Threshold 40 (Naikkan jika burung masih terdeteksi)
MicrowaveRadar radar(D2, 40); 
const int speakerPin = D8;

void setup() {
  Serial.begin(115200);
  radar.begin();
  pinMode(speakerPin, OUTPUT);
}

void loop() {
  // Update filter radar
  radar.process();

  // Logika Aktuator di luar library (Clean Architecture)
  if (radar.detected()) {
    tone(speakerPin, 1000); 
    Serial.printf("Gerakan Terdeteksi! Strength: %d\n", radar.getSignalStrength());
  } else {
    noTone(speakerPin);
  }

  delay(10); // Sampling rate sekitar 100Hz cukup untuk ESP8266
}
