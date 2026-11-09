#!/usr/bin/env bash
# Compile and upload BeatBlocks sound sketches to an ESP32-S3.
# Usage: ./sound/flash.sh [sketch_name]
# Default sketch: drum_samples
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SKETCH_NAME="${1:-drum_samples}"
SKETCH="$ROOT/sound/$SKETCH_NAME"

# DevKitC-1-N8R8 defaults: 8MB flash, native USB CDC, slower upload for reliability.
FQBN="${FQBN:-esp32:esp32:esp32s3:UploadSpeed=115200,USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=8M,PSRAM=opi,PartitionScheme=default_8MB}"
PORT="${PORT:-}"
MONITOR="${MONITOR:-1}"

detect_port() {
  if [[ -n "$PORT" ]]; then
    echo "$PORT"
    return
  fi

  local ports
  ports="$(arduino-cli board list 2>/dev/null | awk '/usbmodem|usbserial|wchusbserial|SLAB_USBtoUART|ttyACM|ttyUSB/ {print $1; exit}')"
  if [[ -z "$ports" ]]; then
    ports="$(ls /dev/cu.usbmodem* /dev/cu.usbserial* /dev/cu.wchusbserial* 2>/dev/null | head -n 1 || true)"
  fi

  if [[ -z "$ports" ]]; then
    echo "No ESP32 serial port found. Plug in USB, then rerun." >&2
    echo "Or set PORT=/dev/cu.usbmodemXXXX ./sound/flash.sh" >&2
    exit 1
  fi

  echo "$ports"
}

PORT="$(detect_port)"
echo "Board port: $PORT"
echo "FQBN:       $FQBN"
echo "Sketch:     $SKETCH"
echo
echo "If upload fails: hold BOOT, tap RESET, release BOOT, then rerun."
echo

arduino-cli compile --fqbn "$FQBN" "$SKETCH"
arduino-cli upload -p "$PORT" --fqbn "$FQBN" "$SKETCH"

echo
echo "Upload complete."

if [[ "$MONITOR" == "1" ]]; then
  echo "Opening serial monitor (Ctrl-C to exit)..."
  arduino-cli monitor -p "$PORT" -c baudrate=115200
fi
