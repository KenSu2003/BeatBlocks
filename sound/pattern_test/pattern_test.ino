/*
  BeatBlocks — simple pattern array test
  Board: ESP32-S3
  Amp:   MAX98357A
  Tempo: Bourns PDB181-K420K-103B 10 kΩ pot

  Edit kick[] / snare[] (len=8), then flash.
  1 = block present, 0 = empty
  Two steps per beat (eight steps per loop).
  BPM comes from the tempo pot (50–170).

  Wiring:
    BCLK → GPIO 9
    LRC  → GPIO 8
    DIN  → GPIO 10
    Pot outer → 3.3 V
    Pot wiper → GPIO 4
    Pot outer → GND
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

// --- edit these to test rhythms ---
static const int8_t kick[kNumSteps]  = {1, 1, 0, 1, 1, 0, 1, 1};
static const int8_t snare[kNumSteps] = {1, 0, 1, 0, 1, 0, 1, 0};
// ----------------------------------

I2SClass i2s;
static float bpmFiltered = 96.0f;

static int readBpmFromPot() {
  // Average a few samples to reduce ADC noise.
  uint32_t sum = 0;
  for (int i = 0; i < 8; i++) {
    sum += analogRead(TEMPO_POT_PIN);
  }
  const int raw = (int)(sum / 8);

  // Map full ADC range to BPM. Swap ends on the pot if direction feels reversed.
  const int bpm = map(raw, 0, 4095, kBpmMin, kBpmMax);
  return constrain(bpm, kBpmMin, kBpmMax);
}

static int currentBpm() {
  const int measured = readBpmFromPot();
  // Light smoothing so tempo doesn't chatter every step.
  bpmFiltered = (0.80f * bpmFiltered) + (0.20f * (float)measured);
  return constrain((int)(bpmFiltered + 0.5f), kBpmMin, kBpmMax);
}

static int stepDurationMs(int bpm) {
  // Two steps per beat → step ms = 60000 / bpm / 2
  return 60000 / bpm / 2;
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

  if (!playKick && !playSnare) {
    playSilenceMs(stepMs);
    return;
  }

  if (playKick && !playSnare) {
    size_t i = 0;
    static int16_t buf[128];
    while (i < KICK_NUM_SAMPLES) {
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
    size_t i = 0;
    static int16_t buf[128];
    while (i < SNARE_NUM_SAMPLES) {
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
    // Both on the same step: mix until the shorter ends, then finish the longer.
    const size_t total = max(KICK_NUM_SAMPLES, SNARE_NUM_SAMPLES);
    size_t i = 0;
    static int16_t buf[128];
    while (i < total) {
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
  Serial.println("BeatBlocks pattern test");
  Serial.printf("tempo pot on GPIO %d → %d..%d BPM\n", TEMPO_POT_PIN, kBpmMin, kBpmMax);

  analogReadResolution(12);
  analogSetPinAttenuation(TEMPO_POT_PIN, ADC_11db);
  pinMode(TEMPO_POT_PIN, INPUT);

  Serial.print("kick : [");
  for (int i = 0; i < kNumSteps; i++) {
    Serial.print((int)kick[i]);
    if (i + 1 < kNumSteps) {
      Serial.print(',');
    }
  }
  Serial.println(']');
  Serial.print("snare: [");
  for (int i = 0; i < kNumSteps; i++) {
    Serial.print((int)snare[i]);
    if (i + 1 < kNumSteps) {
      Serial.print(',');
    }
  }
  Serial.println(']');

  bpmFiltered = (float)readBpmFromPot();
  Serial.printf("initial bpm=%d\n", (int)(bpmFiltered + 0.5f));

  i2s.setPins(I2S_BCLK, I2S_LRC, I2S_DIN);
  if (!i2s.begin(kMode, kSampleRate, kBits, kSlot)) {
    Serial.println("Failed to initialize I2S!");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("I2S ready — looping pattern (twist pot to change tempo)");
}

void loop() {
  const int bpm = currentBpm();
  const int stepMs = stepDurationMs(bpm);
  Serial.printf("bpm=%d stepMs=%d\n", bpm, stepMs);

  for (int i = 0; i < kNumSteps; i++) {
    // Re-read tempo each step so the knob responds within one loop.
    const int liveBpm = currentBpm();
    playStep(kick[i] != 0, snare[i] != 0, stepDurationMs(liveBpm));
  }
}
