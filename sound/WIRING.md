# BeatBlocks sound wiring

ESP32-S3 → MAX98357A I²S amp → 4 Ω / 3 W speaker

| Amp pin | ESP32-S3 |
|---------|----------|
| BCLK    | GPIO 9   |
| LRC/WS  | GPIO 8   |
| DIN     | GPIO 10  |
| VIN     | 5 V (or amp-rated supply) |
| GND     | GND      |

Speaker connects to the amp speaker terminals (+ / −).

## Flash sketches

```bash
./sound/flash.sh              # kick + snare sample test (default)
./sound/flash.sh i2s_tone_test  # original 440 Hz beep
```

Or manually:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32s3 sound/drum_samples
arduino-cli upload -p PORT --fqbn esp32:esp32:esp32s3 sound/drum_samples
```

Expected `drum_samples` result: repeating **kick, snare, kick, snare**.

Regenerate PCM headers (optional):

```bash
.venv/bin/python sound/generate_samples.py
```
