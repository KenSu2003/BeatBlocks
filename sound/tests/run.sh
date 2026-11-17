#!/usr/bin/env bash
set -euo pipefail
TEST_DIR="$(cd "$(dirname "$0")" && pwd)"
TEST_BIN="$(mktemp -t beatblocks-sound-test.XXXXXX)"
trap 'rm -f "$TEST_BIN"' EXIT
c++ -std=c++17 -Wall -Wextra -Werror -I "$TEST_DIR/stubs" "$TEST_DIR/sound_sets_test.cpp" -o "$TEST_BIN"
"$TEST_BIN"
node --test "$TEST_DIR/serial.test.mjs"
