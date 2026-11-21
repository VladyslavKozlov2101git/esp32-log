#include <Arduino.h>

// Мінімальний тест серво: розгортка 0-180° і назад.
static const int SERVO_PIN = 23;
static const int MIN_US = 500;
static const int MAX_US = 2400;

void servoWrite(int deg) {
  int us = MIN_US + (long)(MAX_US - MIN_US) * deg / 180;
  ledcWrite(SERVO_PIN, (uint32_t)us * 65535 / 20000);
  Serial.printf("deg=%3d  pulse=%4d us\n", deg, us);
}

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println();
  Serial.println("=== SERVO TEST (GPIO23) ===");
  ledcAttach(SERVO_PIN, 50, 16);
  delay(300);
}

void loop() {
  for (int a = 0; a <= 180; a += 10) {
    servoWrite(a);
    delay(300);
  }
  Serial.println("--- back ---");
  for (int a = 180; a >= 0; a -= 10) {
    servoWrite(a);
    delay(300);
  }
  Serial.println("--- loop ---");
}
