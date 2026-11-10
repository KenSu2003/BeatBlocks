/*
  BeatBlocks — single reed + magnet smoke test
  Board: ESP32-S3

  Wiring (NO reed switch):
    One side → GPIO 15
    Other side → GND
    Use INPUT_PULLUP (or external 10 kΩ to 3.3 V)

  Magnet near the reed → CLOSED (reads LOW)
  Magnet away → OPEN (reads HIGH)
*/

#include <Arduino.h>

#define REED_PIN 15

static bool lastClosed = false;

void setup() {
  Serial.begin(115200);
  delay(400);
  pinMode(REED_PIN, INPUT_PULLUP);

  Serial.println("BeatBlocks single reed test");
  Serial.printf("Reed on GPIO %d (INPUT_PULLUP)\n", REED_PIN);
  Serial.println("Bring a magnet near the switch...");
}

void loop() {
  // LOW = switch closed by magnet (pulled to GND)
  const bool closed = digitalRead(REED_PIN) == LOW;

  if (closed != lastClosed) {
    lastClosed = closed;
    Serial.println(closed ? "CLOSED (magnet present)" : "OPEN (no magnet)");
  }

  delay(20);
}
