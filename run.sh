#!/usr/bin/env bash
# BeatBlocks full bench run: flash reed_bridge (audio + playhead), serve reed_sim UI.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

echo "==> Freeing serial monitor if any..."
pkill -f 'arduino-cli monitor' 2>/dev/null || true
sleep 0.5

echo "==> Flashing sound/reed_bridge (speaker + playhead LEDs + tempo pot)..."
MONITOR=0 "$ROOT/sound/flash.sh" reed_bridge

echo
echo "==> Starting reed_sim UI on http://127.0.0.1:8765"
echo "    Chrome/Edge → Connect ESP32 → click kick/snare cells"
echo "    Close this with Ctrl-C when done."
echo

exec "$ROOT/sound/reed_sim/serve.sh"
