# Reed-switch simulator → ESP32 speaker

Fake the magnetic block grid in the browser. The ESP32 still plays kick/snare on the real amp.

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

Tempo still comes from the Bourns pot on GPIO 4.

## Serial protocol

```
P:1,0,0,0,1,0,0,0;0,0,1,0,0,0,1,0
K:1,0,1,0,1,0,1,0
S:0,1,0,1,0,1,0,1
Q
```

Patterns apply at the next 8-step bar.
