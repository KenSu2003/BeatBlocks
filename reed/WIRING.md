# BeatBlocks reed-switch wiring

Magnetic blocks over reed switches set kick/snare occupancy (len=8 each).
Target: feed the same `kick[]` / `snare[]` arrays used by `sound/reed_bridge`.

## Hardware (from parts list)

- 16× NO reed switches (2 rows × 8 steps)
- Magnets inside PLA tokens
- Direct GPIOs for the 16-switch smoke test (MCP23017 still an option later)
- `INPUT_PULLUP` for bench tests; prefer 10 kΩ pull-ups on the final board

## Do not reuse

| Function | GPIOs |
|----------|--------|
| I²S | **2** (BCLK), **42** (LRC), **1** (DIN) |
| Tempo pot | **14** |
| Playhead LEDs | **21, 47, 48, 45, 38, 39, 40, 41** |

## Full 16-reed map

Each reed: GPIO ↔ NO switch ↔ GND.

| Step | Kick GPIO | Snare GPIO |
|------|-----------|------------|
| 0 | 4 | 12 |
| 1 | 5 | 13 |
| 2 | 6 | 3 |
| 3 | 7 | 15 |
| 4 | 8 | 16 |
| 5 | 9 | 17 |
| 6 | 10 | 18 |
| 7 | 11 | 46 |

```bash
./reed/flash.sh full16
```

Serial prints `K[...]` / `S[...]` whenever occupancy changes.

## Tempo pot (shared with sound)

Wiper → **GPIO 14** (3.3 V / GND on the outer pins).

## Smaller smoke tests

```bash
./reed/flash.sh single_reed   # GPIO 15 only
./reed/flash.sh triple_reed   # GPIO 4,5,6
```

## Rows

| Row | Steps | Maps to |
|-----|-------|---------|
| Kick (top) | 0–7 | `kick[8]` |
| Snare (bottom) | 0–7 | `snare[8]` |

## Build order

1. One reed + magnet smoke test
2. Triple reed + spacing check
3. Full 16 occupancy map
4. Merge occupancy into `reed_bridge` (audio + playhead)
