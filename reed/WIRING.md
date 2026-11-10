# BeatBlocks reed-switch wiring

Magnetic blocks over reed switches set kick/snare occupancy (len=8 each).
Target: feed the same `kick[]` / `snare[]` arrays used by `sound/reed_bridge`.

## Hardware (from parts list)

- 16× NO reed switches (2 rows × 8 steps)
- Magnets inside PLA tokens
- MCP23017 I²C GPIO expander (14 inputs) + 2× ESP32 GPIO for the remaining switches
- 10 kΩ pull-ups on each reed input

## Single-reed smoke test (first)

| Signal | ESP32-S3 |
|--------|----------|
| Reed side A | **GPIO 15** |
| Reed side B | **GND** |

Sketch uses `INPUT_PULLUP` (no external resistor required for the first test).  
Optional: 10 kΩ pull-up from GPIO 15 to 3.3 V instead.

```bash
./reed/flash.sh single_reed
```

Expect Serial: `CLOSED (magnet present)` / `OPEN (no magnet)`.

## Planned I²C

| Signal | ESP32-S3 | Notes |
|--------|----------|--------|
| SDA | TBD | Keep clear of I²S 8/9/10, pot 4, playhead LEDs, reed 15 |
| SCL | TBD | |
| MCP23017 ADDR | TBD | |

## Rows

| Row | Steps | Maps to |
|-----|-------|---------|
| Kick (top) | 0–7 | `kick[8]` |
| Snare (bottom) | 0–7 | `snare[8]` |

## Build order

1. One reed + magnet smoke test
2. MCP23017 bring-up
3. 2×2 fixture + debounce / neighbor check
4. Expand to 16, then merge occupancy into `reed_bridge`

See project timeline: Tianji owns reed grid; Ken owns audio + playhead.
