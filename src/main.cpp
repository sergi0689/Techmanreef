#include <Arduino.h>

// Firmware de prueba para validar la compilación en GitHub Actions.
// NO controla Evergrow ni Jebao y NO incluye OTA.

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Techman Reef | ESP32 | compilacion de prueba");
  Serial.println("AVISO: firmware provisional sin radio RF ni OTA");
}

void loop() {
  delay(1000);
}
