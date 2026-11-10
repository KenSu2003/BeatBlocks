/*
  BeatBlocks — three reed + magnet smoke test
  Board: ESP32-S3

  Wiring (NO reed switch each):
    One side → GPIO 4 / 5 / 6
    Other side → GND
    INPUT_PULLUP (or external 10 kΩ to 3.3 V)

  Magnet near reed → CLOSED (LOW)
*/

#include <Arduino.h>

static const int kNumReeds = 3;
static const int kReedPins[kNumReeds] = {4, 5, 6};
static bool lastClosed[kNumReeds] = {false, false, false};

void setup() {
  Serial.begin(115200);
  delay(400);

  Serial.println("BeatBlocks triple reed test");
  for (int i = 0; i < kNumReeds; i++) {
    pinMode(kReedPins[i], INPUT_PULLUP);
    Serial.printf("reed[%d] → GPIO %d\n", i, kReedPins[i]);
  }
  Serial.println("Bring magnets near the switches...");
}

void loop() {
  for (int i = 0; i < kNumReeds; i++) {
    const bool closed = digitalRead(kReedPins[i]) == LOW;
    if (closed != lastClosed[i]) {
      lastClosed[i] = closed;
      Serial.printf("reed[%d] GPIO %d → %s\n", i, kReedPins[i],
                    closed ? "CLOSED" : "OPEN");
    }
  }
  delay(20);
}
