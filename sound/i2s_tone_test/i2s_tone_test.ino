/*
  BeatBlocks — I2S amp + speaker smoke test
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

#define I2S_BCLK 2
#define I2S_LRC  42
#define I2S_DIN  1

static const int kFrequencyHz = 440;
static const int kAmplitude = 4000;
static const int kSampleRate = 16000;
static const int kBeepMs = 120;
static const int kGapMs = 880;

static const i2s_data_bit_width_t kBits = I2S_DATA_BIT_WIDTH_16BIT;
static const i2s_mode_t kMode = I2S_MODE_STD;
static const i2s_slot_mode_t kSlot = I2S_SLOT_MODE_STEREO;

I2SClass i2s;

static void playToneMs(int durationMs) {
  const unsigned halfWave = kSampleRate / kFrequencyHz / 2;
  const size_t totalFrames = (size_t)kSampleRate * (size_t)durationMs / 1000;

  int16_t sample = kAmplitude;
  unsigned count = 0;

  for (size_t frame = 0; frame < totalFrames; frame++) {
    if (count % halfWave == 0) {
      sample = (int16_t)(-sample);
    }

    int16_t left = sample;
    int16_t right = sample;
    i2s.write(&left, sizeof(left));
    i2s.write(&right, sizeof(right));
    count++;
  }
}

static void playSilenceMs(int durationMs) {
  const size_t totalFrames = (size_t)kSampleRate * (size_t)durationMs / 1000;
  int16_t zero = 0;

  for (size_t frame = 0; frame < totalFrames; frame++) {
    i2s.write(&zero, sizeof(zero));
    i2s.write(&zero, sizeof(zero));
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("BeatBlocks I2S tone test");
  Serial.printf("BCLK=%d  LRC=%d  DIN=%d\n", I2S_BCLK, I2S_LRC, I2S_DIN);

  i2s.setPins(I2S_BCLK, I2S_LRC, I2S_DIN);

  if (!i2s.begin(kMode, kSampleRate, kBits, kSlot)) {
    Serial.println("Failed to initialize I2S!");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("I2S ready — beeping 440 Hz once per second");
}

void loop() {
  playToneMs(kBeepMs);
  playSilenceMs(kGapMs);
}
