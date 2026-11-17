# Selectable sound sets

| Set | Top row | Bottom row |
|---|---|---|
| Drums (default) | Kick | Snare |
| Percussion | Tom | Clap |
| Wood | Woodblock | Claves |

The blocks still choose **when** notes play. The sound set chooses **which instruments** play them. Changing sets keeps the pattern and tempo.

## Browser demo

Run the local setup in [software/README.md](../software/README.md). Select **Sound set** above the board. During playback, changes begin at a loop boundary. **Save my beat** stores the set too. In a shared session, either participant can change it; both browsers use the server's scheduled boundary. Older saved beats use Drums.

The original browser kick/snare synthesis is unchanged. The four new sounds use the same PCM samples as the board. GitHub contains the updated code; the existing hosted demo/API must be deployed separately to expose these changes at its public URL.

## Physical board

1. Close any serial monitor. From the repository root, flash once:
   ```sh
   MONITOR=0 ./sound/flash.sh reed_bridge
   ```
   Requires `arduino-cli` on PATH and the ESP32 core. Verified with core **3.3.2** and the existing ESP32-S3 N8R8 board settings in the script.
2. Run `./sound/reed_sim/serve.sh`; open **http://127.0.0.1:8765** in Chrome/Edge.
3. Click **Connect ESP32** and choose the board. The selector enables when the board reports its set.
4. Choose a set. **Queued** means it will change at the next eight-step loop; **Active** confirms the board applied it.

The page initially sends its example pattern. Click **Clear** to test physical blocks alone; occupied reeds and enabled browser cells are OR-merged. The tempo pot, LEDs, GPIO wiring, and original hardware kick/snare PCM are unchanged. The board starts in Drums after reset. Disconnecting USB control leaves the running set active while the board remains powered.

USB commands: `KIT:drums`, `KIT:percussion`, `KIT:wood`, each followed by a newline. `Q` reports `KIT:<active-id>`. Existing `K:`, `S:`, and `P:` pattern commands still work and apply at the next step. Sound-set changes apply at step zero.

The physical board is controlled over USB. Shared online sessions currently synchronize browsers; this firmware does not add Wi-Fi or synchronize the board clock with them.

## Checks

```sh
node --test software/tests/*.test.mjs
bash sound/tests/run.sh
cd software && npm run build
```

Verified: **43 JavaScript tests**, the native mixer suite, the production web build, and an ESP32-S3 compile with core 3.3.2. The firmware uses **470,179 bytes of flash** and **22,336 bytes of static RAM**. Serial regression tests cover malformed input and delayed sound-set acknowledgements.

The firmware uses a fixed eight-voice mixer so sample tails can overlap subsequent hits. An isolated kick/snare hit renders its original PCM values exactly. Dense mixes saturate at the signed 16-bit range. The audio queue keeps the newest hit if I²S stalls, avoiding a delayed burst.

**Bench acceptance still required:**

- Start in Drums. Place a few blocks and confirm the original kick/snare sounds and positions.
- Switch Drums → Percussion → Wood → Drums without moving blocks. Confirm each change starts at position 1 and returns to the original sounds.
- Play both rows on every step at 50, 96, and 170 BPM; listen for missing hits, harsh clipping, and glitches.
- Disconnect browser control while keeping the board powered; check sound, reeds, tempo, and LEDs continue working.
- Reset the board; confirm Drums is restored. Reconnect and check the displayed active set.

## Adding a sound later

Edit the catalog in `software/lib/sound-catalog.mjs`, add sample generation in `software/scripts/generate-sound-sets.mjs`, and extend `sound/reed_bridge/SoundBank.h`. Regenerate with:

```sh
node software/scripts/generate-sound-sets.mjs
node software/scripts/sync-demo.mjs
```

The generator preserves the existing hardware drum samples and creates matching browser/firmware assets. Re-run the checks above after generation.
