/*
  BeatBlocks — kick + snare sample playback test
  Board: ESP32-S3
  Amp:   MAX98357A

  Wiring:
    BCLK → GPIO 2
    LRC  → GPIO 42
    DIN  → GPIO 1
    VIN / GND powered to the amp
*/

#include <Arduino.h>
#include <ESP_I2S.h>

#include "kick_pcm.h"
#include "snare_pcm.h"

#define I2S_BCLK 2
#define I2S_LRC  42
#define I2S_DIN  1

#if KICK_SAMPLE_RATE != SNARE_SAMPLE_RATE
#error "kick and snare sample rates must match"
#endif

static const int kSampleRate = KICK_SAMPLE_RATE;
static const i2s_data_bit_width_t kBits = I2S_DATA_BIT_WIDTH_16BIT;
static const i2s_mode_t kMode = I2S_MODE_STD;
static const i2s_slot_mode_t kSlot = I2S_SLOT_MODE_STEREO;

// Simple check pattern: kick, snare, kick, snare @ ~96 BPM eighths.
static const int kStepMs = 156;
static const int kGapAfterHitMs = 10;

I2SClass i2s;

static int16_t readPgmSample(const int16_t *pcm, size_t index) {
  return (int16_t)pgm_read_word(&pcm[index]);
}

static void playMonoSample(const int16_t *pcm, size_t numSamples) {
  // Stream in small stereo frames: L=R=sample.
  static int16_t frame[256];
  size_t i = 0;
  while (i < numSamples) {
    size_t n = numSamples - i;
    if (n > 128) {
      n = 128;
    }

    for (size_t s = 0; s < n; s++) {
      const int16_t v = readPgmSample(pcm, i + s);
      frame[s * 2] = v;
      frame[s * 2 + 1] = v;
    }

    i2s.write(frame, n * 2 * sizeof(int16_t));
    i += n;
  }
}

static void playSilenceMs(int durationMs) {
  if (durationMs <= 0) {
    return;
  }

  const size_t totalFrames = (size_t)kSampleRate * (size_t)durationMs / 1000;
  static int16_t zeros[256];
  memset(zeros, 0, sizeof(zeros));

  size_t written = 0;
  while (written < totalFrames) {
    size_t n = totalFrames - written;
    if (n > 128) {
      n = 128;
    }
    i2s.write(zeros, n * 2 * sizeof(int16_t));
    written += n;
  }
}

static void playHit(const int16_t *pcm, size_t numSamples) {
  const unsigned long startMs = millis();
  playMonoSample(pcm, numSamples);

  const int elapsed = (int)(millis() - startMs);
  const int remain = kStepMs - elapsed - kGapAfterHitMs;
  if (remain > 0) {
    playSilenceMs(remain);
  }
  playSilenceMs(kGapAfterHitMs);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("BeatBlocks kick + snare sample test");
  Serial.printf("BCLK=%d  LRC=%d  DIN=%d  rate=%d\n", I2S_BCLK, I2S_LRC, I2S_DIN, kSampleRate);
  Serial.printf("kick=%u samples  snare=%u samples\n", (unsigned)KICK_NUM_SAMPLES, (unsigned)SNARE_NUM_SAMPLES);

  i2s.setPins(I2S_BCLK, I2S_LRC, I2S_DIN);
  if (!i2s.begin(kMode, kSampleRate, kBits, kSlot)) {
    Serial.println("Failed to initialize I2S!");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("I2S ready — pattern: kick, snare, kick, snare");
}

void loop() {
  playHit(KICK_PCM, KICK_NUM_SAMPLES);
  playHit(SNARE_PCM, SNARE_NUM_SAMPLES);
  playHit(KICK_PCM, KICK_NUM_SAMPLES);
  playHit(SNARE_PCM, SNARE_NUM_SAMPLES);
}
