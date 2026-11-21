#include <Arduino.h>

// Точний тест типу серво: зсув на 20 мкс від центру (1500).
// Позиційне — зрушить на кілька градусів і зупиниться.
// Безперервне (або зламаний потенціометр) — крутитиметься без кінця.
static const int SERVO_PIN = 23;

void pulseUs(int us) {
  Serial.printf("pulse=%d us\n", us);
  ledcWrite(SERVO_PIN, (uint32_t)us * 65535 / 20000);
}

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println();
  Serial.println("=== SERVO TYPE TEST v2 ===");
  ledcAttach(SERVO_PIN, 50, 16);
  delay(300);
}

void loop() {
  Serial.println("-> 1520 us на 8 с (має ледь зрушити й стати)");
  pulseUs(1520);
  delay(8000);

  Serial.println("-> 1480 us на 8 с (має ледь зрушити назад і стати)");
  pulseUs(1480);
  delay(8000);
}
