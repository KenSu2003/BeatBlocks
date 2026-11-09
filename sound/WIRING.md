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

## Tempo pot (Bourns PDB181-K420K-103B, 10 kΩ)

| Pot pin | ESP32-S3 |
|---------|----------|
| Outer 1 | **3.3 V** (not 5 V) |
| Center (wiper) | **GPIO 4** |
| Outer 2 | **GND** |

If turning the knob makes tempo go the wrong way, swap the two outer wires.

## Flash sketches

```bash
./sound/flash.sh                 # pattern_test (default): kick[] + snare[] len=8
./sound/flash.sh drum_samples    # alternating kick/snare hits
./sound/flash.sh i2s_tone_test   # original 440 Hz beep
```

Edit the arrays in `sound/pattern_test/pattern_test.ino`:

```cpp
const int8_t kick[8]  = {1, 0, 0, 0, 1, 0, 0, 0};
const int8_t snare[8] = {0, 0, 1, 0, 0, 0, 1, 0};
// BPM comes from the tempo pot on GPIO 4 (50–170)
```

Regenerate PCM headers (optional; writes into both sketch folders):

```bash
.venv/bin/python sound/generate_samples.py
```
