# BeatBlocks lighting wiring

## Playhead LEDs (8 steps)

One LED per step column. Light the current step while the sequencer runs.

| Step | ESP32 GPIO | Notes |
|------|------------|--------|
| 0 | TBD | LED + series resistor → GPIO, other side → GND |
| 1 | TBD | |
| 2 | TBD | |
| 3 | TBD | |
| 4 | TBD | |
| 5 | TBD | |
| 6 | TBD | |
| 7 | TBD | |

Avoid pins already used by sound:

- I²S: GPIO 8 (LRC), 9 (BCLK), 10 (DIN)
- Tempo pot: GPIO 4

Suggested free set (update when wired): **1, 2, 5, 6, 7, 11, 12, 13**

## Occupancy LEDs (later)

Up to 16 LEDs under kick/snare slots — prefer MCP23017 with the reed grid.
