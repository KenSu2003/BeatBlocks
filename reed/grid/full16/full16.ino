/*
  BeatBlocks — 16 reed occupancy map smoke test
  Board: ESP32-S3

  Two rows × 8 steps → kick[8] / snare[8]
  Each reed: GPIO ↔ NO switch ↔ GND (INPUT_PULLUP)

  Kick  GPIOs:  4, 5, 6, 7, 8, 9, 10, 11
  Snare GPIOs: 12, 13, 3, 15, 16, 17, 18, 46
*/

#include <Arduino.h>

static const int kNumSteps = 8;
static const int kKickPins[kNumSteps]  = {4, 5, 6, 7, 8, 9, 10, 11};
static const int kSnarePins[kNumSteps] = {12, 13, 3, 15, 16, 17, 18, 46};

static int8_t kick[kNumSteps];
static int8_t snare[kNumSteps];
static int8_t lastKick[kNumSteps];
static int8_t lastSnare[kNumSteps];

static int8_t readReed(int pin) {
  // LOW = magnet present (switch closed to GND)
  return digitalRead(pin) == LOW ? 1 : 0;
}

static void printRow(const char *label, const int8_t *row) {
  Serial.print(label);
  Serial.print('[');
  for (int i = 0; i < kNumSteps; i++) {
    Serial.print((int)row[i]);
    if (i + 1 < kNumSteps) {
      Serial.print(',');
    }
  }
  Serial.println(']');
}

static bool rowsEqual(const int8_t *a, const int8_t *b) {
  for (int i = 0; i < kNumSteps; i++) {
    if (a[i] != b[i]) {
      return false;
    }
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println("BeatBlocks full16 reed map");
  Serial.println("Kick : GPIO 4,5,6,7,8,9,10,11");
  Serial.println("Snare: GPIO 12,13,3,15,16,17,18,46");

  for (int i = 0; i < kNumSteps; i++) {
    pinMode(kKickPins[i], INPUT_PULLUP);
    pinMode(kSnarePins[i], INPUT_PULLUP);
    kick[i] = 0;
    snare[i] = 0;
    lastKick[i] = -1;
    lastSnare[i] = -1;
  }
}

void loop() {
  for (int i = 0; i < kNumSteps; i++) {
    kick[i] = readReed(kKickPins[i]);
    snare[i] = readReed(kSnarePins[i]);
  }

  if (!rowsEqual(kick, lastKick) || !rowsEqual(snare, lastSnare)) {
    memcpy(lastKick, kick, sizeof(kick));
    memcpy(lastSnare, snare, sizeof(snare));
    printRow("K", kick);
    printRow("S", snare);
    Serial.println("---");
  }

  delay(20);
}
