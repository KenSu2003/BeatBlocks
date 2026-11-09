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

## Flash the tone test

```bash
arduino-cli compile --fqbn esp32:esp32:esp32s3 sound/i2s_tone_test
arduino-cli upload -p PORT --fqbn esp32:esp32:esp32s3 sound/i2s_tone_test
arduino-cli monitor -p PORT -c baudrate=115200
```

Replace `PORT` with your board port (for example `/dev/cu.usbmodem101`).

Expected result: a short 440 Hz beep about once per second.
