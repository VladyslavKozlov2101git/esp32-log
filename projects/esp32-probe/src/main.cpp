#include <Arduino.h>

// Сканер напруг: читає всі входи ADC1 одночасно.
// Не важливо, у який пін вставлений щуп — побачимо, де напруга.
static const int PINS[] = {32, 33, 34, 35, 36, 39};
static const int NP = sizeof(PINS) / sizeof(PINS[0]);
static const int TRIG_PIN = 18;

void setup() {
  Serial.begin(115200);
  delay(400);
  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);
  for (int i = 0; i < NP; i++) {
    analogSetPinAttenuation(PINS[i], ADC_11db);
  }
  Serial.println();
  Serial.println("=== PROBE V2 (multi-pin) ===");
}

void loop() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  int mn[NP], mx[NP];
  for (int i = 0; i < NP; i++) {
    mn[i] = 5000;
    mx[i] = 0;
  }
  unsigned long t0 = micros();
  while (micros() - t0 < 40000) {
    for (int i = 0; i < NP; i++) {
      int v = analogReadMilliVolts(PINS[i]);
      if (v < mn[i]) mn[i] = v;
      if (v > mx[i]) mx[i] = v;
    }
  }

  for (int i = 0; i < NP; i++) {
    Serial.printf("GPIO%-2d idle=%4d max=%4d   ", PINS[i], mn[i], mx[i]);
  }
  Serial.println();
  delay(600);
}
