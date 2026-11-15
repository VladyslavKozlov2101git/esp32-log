#include <Arduino.h>

// On most ESP32 DevKit (WROOM-32) boards the on-board LED is on GPIO2.
// If your board's LED does not blink, try GPIO 5 or 4.
static const int LED_PIN = 2;

void setup() {
  Serial.begin(115200);
  delay(300);  // give the USB-UART a moment
  Serial.println();
  Serial.println("=== ESP32 blink started ===");
  Serial.printf("Chip: %s | cores: %d | flash: %lu KB\n",
                ESP.getChipModel(), ESP.getChipCores(),
                (unsigned long)(ESP.getFlashChipSize() / 1024));
  Serial.printf("Free heap: %lu bytes\n", (unsigned long)ESP.getFreeHeap());

  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED ON");
  delay(1000);

  digitalWrite(LED_PIN, LOW);
  Serial.println("LED OFF");
  delay(1000);
}
