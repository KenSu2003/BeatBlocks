# BeatBlocks lighting wiring

## Playhead LEDs (8 steps)

One LED per step column. Light the current step while the sequencer runs.

Each LED: **GPIO → 220 Ω–1 kΩ resistor → LED anode → LED cathode → GND**

| Step | ESP32 GPIO |
|------|------------|
| 0 | 47 |
| 1 | 48 |
| 2 | 45 |
| 3 | 38 |
| 4 | 39 |
| 5 | 40 |
| 6 | 41 |
| 7 | 42 |

Chase order: **47 → 48 → 45 → 38 → 39 → 40 → 41 → 42**

Pins already used by sound (do not reuse):

- I²S: GPIO 8 (LRC), 9 (BCLK), 10 (DIN)
- Tempo pot: GPIO 4

If GPIO 45/46 misbehave (strapping pins on some S3 boards), remapping those two steps is easy in firmware.

## Flash playhead chase test

```bash
MONITOR=0 ./lighting/flash.sh playhead
```

## Combined with sound

Playhead LEDs are also driven by `sound/reed_bridge` while audio plays:

```bash
MONITOR=0 ./sound/flash.sh reed_bridge
./sound/reed_sim/serve.sh
```

## Occupancy LEDs (later)

Up to 16 LEDs under kick/snare slots — prefer MCP23017 with the reed grid.
