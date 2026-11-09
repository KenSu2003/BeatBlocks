#!/usr/bin/env bash
# Serve the reed-switch simulator UI on http://127.0.0.1:8765
set -euo pipefail
cd "$(dirname "$0")"
echo "Open http://127.0.0.1:8765"
echo "Close Arduino serial monitor before clicking Connect."
python3 -m http.server 8765 --bind 127.0.0.1
