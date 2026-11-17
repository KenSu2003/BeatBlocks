/*
  BeatBlocks — integrated sequencer firmware
  Plays selectable instruments from physical reeds OR reed_sim UI (OR-merged).
  UI: sound/reed_sim (Chrome Web Serial)

  Protocol (newline-terminated):
    KIT:drums / KIT:percussion / KIT:wood (applies at step zero)
    K:1,0,1,0,1,0,1,0
    S:0,1,0,1,0,1,0,1
    P:1,0,1,0,1,0,1,0;0,1,0,1,0,1,0,1   (kick;snare atomic)

  UI patterns apply at the next step.
  Live hit = reed occupied OR UI cell on.
  Tempo pot on GPIO 14 → 50–170 BPM.
  Playhead LEDs track the current step.

  Wiring:
    BCLK → GPIO 2
    LRC  → GPIO 42
    DIN  → GPIO 1
    Pot → 3.3V / GPIO 14 / GND
    Playhead LEDs (step 0..7):
      21, 47, 48, 45, 38, 39, 40, 41
      each: GPIO → resistor → LED → GND
    Kick reeds:  4, 5, 6, 7, 8, 9, 10, 11
    Snare reeds: 12, 13, 3, 15, 16, 17, 18, 46
      each: GPIO ↔ NO reed ↔ GND (INPUT_PULLUP; LOW = magnet)
*/

#include <Arduino.h>
#include <ESP_I2S.h>

#include "SoundBank.h"
#include "SampleMixer.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#define I2S_BCLK 2
#define I2S_LRC  42
#define I2S_DIN  1
#define TEMPO_POT_PIN 14

#if KICK_SAMPLE_RATE != SNARE_SAMPLE_RATE
#error "kick and snare sample rates must match"
#endif

static const int kSampleRate = KICK_SAMPLE_RATE;
static const int kNumSteps = 8;
static const int kBpmMin = 50;
static const int kBpmMax = 170;
static const int kLedPins[kNumSteps] = {21, 47, 48, 45, 38, 39, 40, 41};
static const int kKickReedPins[kNumSteps] = {4, 5, 6, 7, 8, 9, 10, 11};
static const int kSnareReedPins[kNumSteps] = {12, 13, 3, 15, 16, 17, 18, 46};
static const i2s_data_bit_width_t kBits = I2S_DATA_BIT_WIDTH_16BIT;
static const i2s_mode_t kMode = I2S_MODE_STD;
static const i2s_slot_mode_t kSlot = I2S_SLOT_MODE_STEREO;

static void setPlayhead(int step) {
  for (int i = 0; i < kNumSteps; i++) {
    digitalWrite(kLedPins[i], (i == step) ? HIGH : LOW);
  }
}

static void setupPlayheadLeds() {
  for (int i = 0; i < kNumSteps; i++) {
    pinMode(kLedPins[i], OUTPUT);
    digitalWrite(kLedPins[i], LOW);
  }
}

static void setupReedPins() {
  for (int i = 0; i < kNumSteps; i++) {
    pinMode(kKickReedPins[i], INPUT_PULLUP);
    pinMode(kSnareReedPins[i], INPUT_PULLUP);
  }
}

static int8_t readReed(int pin) {
  return digitalRead(pin) == LOW ? 1 : 0;
}

// UI / serial pattern (reed_sim)
static int8_t uiKick[kNumSteps] = {0, 0, 0, 0, 0, 0, 0, 0};
static int8_t uiSnare[kNumSteps] = {0, 0, 0, 0, 0, 0, 0, 0};
// Physical reed occupancy
static int8_t reedKick[kNumSteps];
static int8_t reedSnare[kNumSteps];
static int8_t lastReedKick[kNumSteps];
static int8_t lastReedSnare[kNumSteps];
static int8_t pendingKick[kNumSteps];
static int8_t pendingSnare[kNumSteps];
static volatile bool patternPending = false;

static void scanReeds() {
  for (int i = 0; i < kNumSteps; i++) {
    reedKick[i] = readReed(kKickReedPins[i]);
    reedSnare[i] = readReed(kSnareReedPins[i]);
  }
}

static bool rowsEqual(const int8_t *a, const int8_t *b) {
  for (int i = 0; i < kNumSteps; i++) {
    if (a[i] != b[i]) {
      return false;
    }
  }
  return true;
}

static void printPattern(const char *label, const int8_t *row);

static void serialLog(const char *msg) {
  // USB CDC Serial.print can block forever if nothing is reading —
  // that froze the playhead when magnets triggered reed logs.
  if (Serial.availableForWrite() < 64) {
    return;
  }
  Serial.println(msg);
}

static void reportReedsIfChanged() {
  if (rowsEqual(reedKick, lastReedKick) && rowsEqual(reedSnare, lastReedSnare)) {
    return;
  }
  memcpy(lastReedKick, reedKick, sizeof(lastReedKick));
  memcpy(lastReedSnare, reedSnare, sizeof(lastReedSnare));
  if (Serial.availableForWrite() < 64) {
    return;
  }
  Serial.print("REED ");
  printPattern("K", reedKick);
  Serial.print("REED ");
  printPattern("S", reedSnare);
}

I2SClass i2s;
static volatile bool audioReady = false;
struct AudioHit { bool top; bool bottom; beatblocks::SoundSet soundSet; };
static QueueHandle_t hitQueue=nullptr;
static beatblocks::SoundSetState soundSetState;
static bool kitReportPending = false;
static float bpmFiltered = 96.0f;
static char lineBuf[96];
static size_t lineLen = 0;
static bool discardLine = false;

static int readBpmFromPot() {
  uint32_t sum = 0;
  for (int i = 0; i < 8; i++) {
    sum += analogRead(TEMPO_POT_PIN);
  }
  const int raw = (int)(sum / 8);
  const int bpm = map(raw, 0, 4095, kBpmMin, kBpmMax);
  return constrain(bpm, kBpmMin, kBpmMax);
}

static int currentBpm() {
  const int measured = readBpmFromPot();
  bpmFiltered = (0.80f * bpmFiltered) + (0.20f * (float)measured);
  return constrain((int)(bpmFiltered + 0.5f), kBpmMin, kBpmMax);
}

static int stepDurationMs(int bpm) {
  return 60000 / bpm / 2;
}

static void reportActiveSoundSet() {
  if (!kitReportPending || Serial.availableForWrite() < 64) {
    return;
  }
  Serial.printf("OK live KIT:%s\n", beatblocks::soundSetId(soundSetState.active));
  kitReportPending = false;
}

static void printPattern(const char *label, const int8_t *row) {
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

static bool parseRow(const char *text, int8_t *out) {
  int idx = 0;
  const char *p = text;
  while (*p && idx < kNumSteps) {
    while (*p == ' ' || *p == ',') {
      p++;
    }
    if (*p != '0' && *p != '1') {
      return false;
    }
    out[idx++] = (int8_t)(*p - '0');
    p++;
  }
  return idx == kNumSteps;
}

static void queuePattern(const int8_t *k, const int8_t *s) {
  memcpy(pendingKick, k, sizeof(pendingKick));
  memcpy(pendingSnare, s, sizeof(pendingSnare));
  patternPending = true;
  if (Serial.availableForWrite() >= 64) {
    Serial.print("OK queued ");
    printPattern("K", pendingKick);
    Serial.print("OK queued ");
    printPattern("S", pendingSnare);
  }
}

static void applyPendingIfAny() {
  if (!patternPending) {
    return;
  }
  noInterrupts();
  memcpy(uiKick, pendingKick, sizeof(uiKick));
  memcpy(uiSnare, pendingSnare, sizeof(uiSnare));
  patternPending = false;
  interrupts();
  if (Serial.availableForWrite() >= 64) {
    Serial.print("OK live UI ");
    printPattern("K", uiKick);
    Serial.print("OK live UI ");
    printPattern("S", uiSnare);
  }
}

static void handleLine(char *line) {
  while (*line == ' ' || *line == '\t') {
    line++;
  }
  if (line[0] == '\0') {
    return;
  }

  if (strncmp(line,"KIT:",4)==0) {
    if(!soundSetState.queueCommand(line)) { serialLog("ERR unknown sound set");return; }
    if(Serial.availableForWrite()>=64) Serial.printf("OK queued KIT:%s\n",beatblocks::soundSetId(soundSetState.pending));
    return;
  }

  if ((line[0] == 'K' || line[0] == 'k') && line[1] == ':') {
    int8_t row[kNumSteps];
    if (!parseRow(line + 2, row)) {
      Serial.println("ERR bad K row");
      return;
    }
    int8_t nextSnare[kNumSteps];
    memcpy(nextSnare, patternPending ? pendingSnare : uiSnare, sizeof(nextSnare));
    queuePattern(row, nextSnare);
    return;
  }

  if ((line[0] == 'S' || line[0] == 's') && line[1] == ':') {
    int8_t row[kNumSteps];
    if (!parseRow(line + 2, row)) {
      Serial.println("ERR bad S row");
      return;
    }
    int8_t nextKick[kNumSteps];
    memcpy(nextKick, patternPending ? pendingKick : uiKick, sizeof(nextKick));
    queuePattern(nextKick, row);
    return;
  }

  if ((line[0] == 'P' || line[0] == 'p') && line[1] == ':') {
    char *semi = strchr(line + 2, ';');
    if (!semi) {
      Serial.println("ERR P needs kick;snare");
      return;
    }
    *semi = '\0';
    int8_t kRow[kNumSteps];
    int8_t sRow[kNumSteps];
    if (!parseRow(line + 2, kRow) || !parseRow(semi + 1, sRow)) {
      Serial.println("ERR bad P rows");
      return;
    }
    queuePattern(kRow, sRow);
    return;
  }

  if ((line[0] == 'Q' || line[0] == 'q') && line[1] == '\0') {
    Serial.printf("KIT:%s\n",beatblocks::soundSetId(soundSetState.active));
    printPattern("UIK", uiKick);
    printPattern("UIS", uiSnare);
    printPattern("RK", reedKick);
    printPattern("RS", reedSnare);
    Serial.printf("BPM %d\n", currentBpm());
    return;
  }

  Serial.println("ERR unknown cmd (use KIT: / K: / S: / P: / Q)");
}

static void pollSerial() {
  while (Serial.available() > 0) {
    const char c = (char)Serial.read();
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      if (!discardLine) {
        lineBuf[lineLen] = '\0';
        handleLine(lineBuf);
      }
      lineLen = 0;
      discardLine = false;
      continue;
    }
    if (discardLine) {
      continue;
    }
    if (c == '\0') {
      discardLine = true;
      lineLen = 0;
      serialLog("ERR invalid line");
      continue;
    }
    if (lineLen + 1 < sizeof(lineBuf)) {
      lineBuf[lineLen++] = c;
    } else {
      lineLen = 0;
      discardLine = true;
      serialLog("ERR line too long");
    }
  }
}

static void writeStereoFrames(const int16_t *mono, size_t n) {
  if (!audioReady) {
    return;
  }
  static int16_t frame[256];
  for (size_t s = 0; s < n; s++) {
    frame[s * 2] = mono[s];
    frame[s * 2 + 1] = mono[s];
  }
  i2s.write(reinterpret_cast<const uint8_t*>(frame), n * 2 * sizeof(int16_t));
}

// Keep playhead moving even when the amp/I2S path is unhappy.
static void waitStepMs(int durationMs) {
  if (durationMs <= 0) {
    return;
  }
  const unsigned long start = millis();
  while ((int)(millis() - start) < durationMs) {
    pollSerial();
    applyPendingIfAny();
    scanReeds();
    delay(1);
  }
}

static void audioTask(void *param) {
  (void)param;
  // Let playhead run a moment before touching I2S.
  vTaskDelay(pdMS_TO_TICKS(1500));

  i2s.setPins(I2S_BCLK, I2S_LRC, I2S_DIN);
  const bool ok = i2s.begin(kMode, kSampleRate, kBits, kSlot);
  audioReady = ok;
  serialLog(ok ? "I2S ready (audio core)" : "I2S failed (LEDs keep running)");

  beatblocks::SampleMixer mixer;
  int16_t buf[128];
  for (;;) {
    if(!audioReady) { vTaskDelay(pdMS_TO_TICKS(10));continue; }
    AudioHit hit;
    if(xQueueReceive(hitQueue,&hit,0)==pdTRUE) {
      if(hit.top) mixer.trigger(beatblocks::sampleFor(hit.soundSet,0));
      if(hit.bottom) mixer.trigger(beatblocks::sampleFor(hit.soundSet,1));
    }
    mixer.render(buf,128);
    writeStereoFrames(buf,128); // I2S/DMA paces audio; main-core sensing remains responsive.
  }
}

void setup() {
  // LEDs first — same path as lighting/playhead (known good).
  setupPlayheadLeds();

  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("BeatBlocks integrated (LEDs + reeds + UI + audio)");
  Serial.println("Hit = reed OR UI cell. Playhead on main core; audio on other core.");

  setupReedPins();
  memset(reedKick, 0, sizeof(reedKick));
  memset(reedSnare, 0, sizeof(reedSnare));
  memset(lastReedKick, 0xFF, sizeof(lastReedKick));
  memset(lastReedSnare, 0xFF, sizeof(lastReedSnare));

  analogReadResolution(12);
  analogSetPinAttenuation(TEMPO_POT_PIN, ADC_11db);
  pinMode(TEMPO_POT_PIN, INPUT);

  memcpy(pendingKick, uiKick, sizeof(pendingKick));
  memcpy(pendingSnare, uiSnare, sizeof(pendingSnare));

  bpmFiltered = (float)readBpmFromPot();

  hitQueue=xQueueCreate(1,sizeof(AudioHit));
  if(hitQueue) xTaskCreatePinnedToCore(audioTask, "audio", 8192, nullptr, 1, nullptr, 0);
  else serialLog("Audio queue allocation failed");
}

void loop() {
  for (int i = 0; i < kNumSteps; i++) {
    setPlayhead(i);
    pollSerial();
    applyPendingIfAny();
    if (soundSetState.atStep(i)) {
      kitReportPending = true;
    }
    reportActiveSoundSet();
    scanReeds();
    reportReedsIfChanged();

    const bool playKick = (uiKick[i] != 0) || (reedKick[i] != 0);
    const bool playSnare = (uiSnare[i] != 0) || (reedSnare[i] != 0);
    if(audioReady && hitQueue && (playKick || playSnare)) {
      const AudioHit hit{playKick,playSnare,soundSetState.active};
      // Retain only the newest event if I2S stalls; never replay a backlog.
      xQueueOverwrite(hitQueue,&hit);
    }

    // Main core never blocks on I2S — only times the playhead.
    waitStepMs(stepDurationMs(currentBpm()));
  }
}
