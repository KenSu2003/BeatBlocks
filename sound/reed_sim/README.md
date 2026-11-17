# Reed-switch simulator → ESP32 speaker

Fake the magnetic block grid in the browser. The ESP32 plays the selected sound set on the real amp: Drums (kick/snare), Percussion (tom/clap), or Wood (woodblock/claves).

## 1. Flash the bridge firmware

```bash
# Stop any serial monitor first (Ctrl-C)
MONITOR=0 ./sound/flash.sh reed_bridge
```

## 2. Open the UI (Chrome or Edge)

```bash
./sound/reed_sim/serve.sh
```

Open [http://127.0.0.1:8765](http://127.0.0.1:8765) → **Connect ESP32** → click kick/snare slots.

Tempo still comes from the Bourns pot on GPIO 14.

## Serial protocol

```
P:1,0,0,0,1,0,0,0;0,0,1,0,0,0,1,0
K:1,0,1,0,1,0,1,0
S:0,1,0,1,0,1,0,1
Q
```

Patterns apply at the next step. Sound sets apply at the next eight-step loop. Use the **Sound set** selector after connecting; the active/queued label follows the board’s acknowledgements.

Commands: `KIT:drums`, `KIT:percussion`, `KIT:wood`. `Q` reports the active set. Click **Clear** to remove the page’s example pattern and hear physical blocks alone.

See [sound-set setup and bench checks](../../docs/SOUND_SETS.md).
