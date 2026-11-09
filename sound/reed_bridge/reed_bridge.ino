/*
  BeatBlocks — reed-switch simulator bridge
  Board plays kick/snare from arrays updated over USB serial.
  UI: sound/reed_sim (Chrome Web Serial)

  Protocol (newline-terminated):
    K:1,0,1,0,1,0,1,0
    S:0,1,0,1,0,1,0,1
    P:1,0,1,0,1,0,1,0;0,1,0,1,0,1,0,1   (kick;snare atomic)

  Patterns apply at the next bar start.
  Tempo pot on GPIO 4 → 50–170 BPM.

  Wiring:
    BCLK → GPIO 9
    LRC  → GPIO 8
    DIN  → GPIO 10
    Pot → 3.3V / GPIO 4 / GND
*/

#include <Arduino.h>
#include <ESP_I2S.h>

#include "kick_pcm.h"
#include "snare_pcm.h"

#define I2S_BCLK 9
#define I2S_LRC  8
#define I2S_DIN  10
#define TEMPO_POT_PIN 4

#if KICK_SAMPLE_RATE != SNARE_SAMPLE_RATE
#error "kick and snare sample rates must match"
#endif

static const int kSampleRate = KICK_SAMPLE_RATE;
static const int kNumSteps = 8;
static const int kBpmMin = 50;
static const int kBpmMax = 170;
static const i2s_data_bit_width_t kBits = I2S_DATA_BIT_WIDTH_16BIT;
static const i2s_mode_t kMode = I2S_MODE_STD;
static const i2s_slot_mode_t kSlot = I2S_SLOT_MODE_STEREO;

static int8_t kick[kNumSteps]  = {1, 0, 0, 0, 1, 0, 0, 0};
static int8_t snare[kNumSteps] = {0, 0, 1, 0, 0, 0, 1, 0};
static int8_t pendingKick[kNumSteps];
static int8_t pendingSnare[kNumSteps];
static volatile bool patternPending = false;

I2SClass i2s;
static float bpmFiltered = 96.0f;
static char lineBuf[96];
static size_t lineLen = 0;

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
  Serial.print("OK queued ");
  printPattern("K", pendingKick);
  Serial.print("OK queued ");
  printPattern("S", pendingSnare);
}

static void applyPendingIfAny() {
  if (!patternPending) {
    return;
  }
  noInterrupts();
  memcpy(kick, pendingKick, sizeof(kick));
  memcpy(snare, pendingSnare, sizeof(snare));
  patternPending = false;
  interrupts();
  Serial.print("OK live ");
  printPattern("K", kick);
  Serial.print("OK live ");
  printPattern("S", snare);
}

static void handleLine(char *line) {
  while (*line == ' ' || *line == '\t') {
    line++;
  }
  if (line[0] == '\0') {
    return;
  }

  if ((line[0] == 'K' || line[0] == 'k') && line[1] == ':') {
    int8_t row[kNumSteps];
    if (!parseRow(line + 2, row)) {
      Serial.println("ERR bad K row");
      return;
    }
    int8_t nextSnare[kNumSteps];
    memcpy(nextSnare, patternPending ? pendingSnare : snare, sizeof(nextSnare));
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
    memcpy(nextKick, patternPending ? pendingKick : kick, sizeof(nextKick));
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
    printPattern("K", kick);
    printPattern("S", snare);
    Serial.printf("BPM %d\n", currentBpm());
    return;
  }

  Serial.println("ERR unknown cmd (use K: / S: / P: / Q)");
}

static void pollSerial() {
  while (Serial.available() > 0) {
    const char c = (char)Serial.read();
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      lineBuf[lineLen] = '\0';
      handleLine(lineBuf);
      lineLen = 0;
      continue;
    }
    if (lineLen + 1 < sizeof(lineBuf)) {
      lineBuf[lineLen++] = c;
    } else {
      lineLen = 0;
      Serial.println("ERR line too long");
    }
  }
}

static int16_t readPgmSample(const int16_t *pcm, size_t index) {
  return (int16_t)pgm_read_word(&pcm[index]);
}

static void writeStereoFrames(const int16_t *mono, size_t n) {
  static int16_t frame[256];
  for (size_t s = 0; s < n; s++) {
    frame[s * 2] = mono[s];
    frame[s * 2 + 1] = mono[s];
  }
  i2s.write(frame, n * 2 * sizeof(int16_t));
}

static void playSilenceFrames(size_t totalFrames) {
  static int16_t zeros[128];
  memset(zeros, 0, sizeof(zeros));
  size_t written = 0;
  while (written < totalFrames) {
    pollSerial();
    size_t n = totalFrames - written;
    if (n > 128) {
      n = 128;
    }
    writeStereoFrames(zeros, n);
    written += n;
  }
}

static void playSilenceMs(int durationMs) {
  if (durationMs <= 0) {
    return;
  }
  playSilenceFrames((size_t)kSampleRate * (size_t)durationMs / 1000);
}

static int16_t mixSample(int16_t a, int16_t b) {
  const int32_t sum = (int32_t)a + (int32_t)b;
  if (sum > 32767) {
    return 32767;
  }
  if (sum < -32768) {
    return -32768;
  }
  return (int16_t)sum;
}

static void playStep(bool playKick, bool playSnare, int stepMs) {
  const unsigned long startMs = millis();
  pollSerial();

  if (!playKick && !playSnare) {
    playSilenceMs(stepMs);
    return;
  }

  size_t i = 0;
  static int16_t buf[128];

  if (playKick && !playSnare) {
    while (i < KICK_NUM_SAMPLES) {
      pollSerial();
      size_t n = KICK_NUM_SAMPLES - i;
      if (n > 128) {
        n = 128;
      }
      for (size_t s = 0; s < n; s++) {
        buf[s] = readPgmSample(KICK_PCM, i + s);
      }
      writeStereoFrames(buf, n);
      i += n;
    }
  } else if (playSnare && !playKick) {
    while (i < SNARE_NUM_SAMPLES) {
      pollSerial();
      size_t n = SNARE_NUM_SAMPLES - i;
      if (n > 128) {
        n = 128;
      }
      for (size_t s = 0; s < n; s++) {
        buf[s] = readPgmSample(SNARE_PCM, i + s);
      }
      writeStereoFrames(buf, n);
      i += n;
    }
  } else {
    const size_t total = max(KICK_NUM_SAMPLES, SNARE_NUM_SAMPLES);
    while (i < total) {
      pollSerial();
      size_t n = total - i;
      if (n > 128) {
        n = 128;
      }
      for (size_t s = 0; s < n; s++) {
        const size_t idx = i + s;
        const int16_t k = (idx < KICK_NUM_SAMPLES) ? readPgmSample(KICK_PCM, idx) : 0;
        const int16_t sn = (idx < SNARE_NUM_SAMPLES) ? readPgmSample(SNARE_PCM, idx) : 0;
        buf[s] = mixSample(k, sn);
      }
      writeStereoFrames(buf, n);
      i += n;
    }
  }

  const int elapsed = (int)(millis() - startMs);
  const int remain = stepMs - elapsed;
  if (remain > 0) {
    playSilenceMs(remain);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("BeatBlocks reed bridge");
  Serial.println("Send K: / S: / P: patterns from reed_sim UI");

  analogReadResolution(12);
  analogSetPinAttenuation(TEMPO_POT_PIN, ADC_11db);
  pinMode(TEMPO_POT_PIN, INPUT);

  memcpy(pendingKick, kick, sizeof(kick));
  memcpy(pendingSnare, snare, sizeof(snare));
  printPattern("K", kick);
  printPattern("S", snare);

  bpmFiltered = (float)readBpmFromPot();
  Serial.printf("initial bpm=%d\n", (int)(bpmFiltered + 0.5f));

  i2s.setPins(I2S_BCLK, I2S_LRC, I2S_DIN);
  if (!i2s.begin(kMode, kSampleRate, kBits, kSlot)) {
    Serial.println("Failed to initialize I2S!");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("I2S ready");
}

void loop() {
  pollSerial();
  applyPendingIfAny();

  for (int i = 0; i < kNumSteps; i++) {
    pollSerial();
    const int liveBpm = currentBpm();
    playStep(kick[i] != 0, snare[i] != 0, stepDurationMs(liveBpm));
  }
}
