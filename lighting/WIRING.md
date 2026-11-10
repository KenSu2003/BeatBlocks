# BeatBlocks lighting wiring

## Playhead LEDs (8 steps)

One LED per step column. Light the current step while the sequencer runs.

Each LED: **GPIO → 220 Ω–1 kΩ resistor → LED anode → LED cathode → GND**

| Step | ESP32 GPIO |
|------|------------|
| 0 | 21 |
| 1 | 47 |
| 2 | 48 |
| 3 | 45 |
| 4 | 38 |
| 5 | 39 |
| 6 | 40 |
| 7 | 41 |

Chase order: **21 → 47 → 48 → 45 → 38 → 39 → 40 → 41**

Pins already used by sound (do not reuse):

- I²S: GPIO 2 (BCLK), 42 (LRC), 1 (DIN)
- Tempo pot: GPIO 14

Note: GPIO **45** is a strapping pin — OK after boot for an LED; if boot acts weird, leave that LED disconnected during reset.

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
