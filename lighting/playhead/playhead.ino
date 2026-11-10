/*
  BeatBlocks — playhead LED chase test
  Board: ESP32-S3

  Walks one-hot across 8 step LEDs.
  Wiring: GPIO → resistor → LED → GND

  Step → GPIO (physical order):
    0:21  1:47  2:48  3:45  4:38  5:39  6:40  7:41
*/

#include <Arduino.h>

static const int kNumSteps = 8;
static const int kLedPins[kNumSteps] = {21, 47, 48, 45, 38, 39, 40, 41};
static const int kStepMs = 200;

static void setPlayhead(int step) {
  for (int i = 0; i < kNumSteps; i++) {
    digitalWrite(kLedPins[i], (i == step) ? HIGH : LOW);
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("BeatBlocks playhead LED test");

  for (int i = 0; i < kNumSteps; i++) {
    pinMode(kLedPins[i], OUTPUT);
    digitalWrite(kLedPins[i], LOW);
    Serial.printf("step %d → GPIO %d\n", i, kLedPins[i]);
  }
}

void loop() {
  for (int step = 0; step < kNumSteps; step++) {
    setPlayhead(step);
    Serial.printf("playhead=%d\n", step);
    delay(kStepMs);
  }
}
