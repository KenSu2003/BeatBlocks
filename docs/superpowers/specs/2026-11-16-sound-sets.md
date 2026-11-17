# BeatBlocks selectable sound sets

Goal: keep the two eight-step block rows and let users change their instruments without rewiring.

Use three preloaded sets: `drums` (Kick / Snare), `percussion` (Tom / Clap), and `wood` (Woodblock / Claves). Drums remains the default. The browser hi-hat track is unchanged. A set changes both row instruments; occupancy and tempo remain unchanged.

Separate occupancy, selected sound set, and sample playback. Preserve the latest development branch's pin map, physical-reed OR browser-pattern behavior, tempo pot, and playhead. Preserve the existing kick/snare PCM data; generate the four new sounds deterministically. Export matching PCM data to the browser. Use a fixed-size mixer so overlapping sample tails do not drop steps at 170 BPM.

The web demo selects sets, saves them locally, and sends them through the existing session API and musical timeline. Shared changes become audible at the existing future loop boundary. Old saved beats/rooms without a sound-set field use drums; unknown set IDs are rejected. Keep the immutable database migration: room state is already JSON.

The USB control page selects a physical board's set through `KIT:<id>`; firmware acknowledges queued and active sets. Apply a pending set at step zero. `Q` reports its active set. Board control remains USB; this change does not add Wi-Fi or synchronize the board clock with shared browser playback. On power-up, the board uses drums.

Approaches considered: per-row arbitrary uploads add decoding and validation; plug-in hardware adds wiring and parts; three preloaded software sets fit the requested scope and existing hardware. This implementation uses the preloaded approach discussed with the user.

Verification: automated session/timing/UI tests, native C++ mixer/protocol tests, deterministic sample checks, full web build, and ESP32 compile if the toolchain is available. Physical audio, reed behavior, and USB connection require a bench check; do not flash attached hardware automatically.

Delivery: implementation plan and concise bench instructions in GitHub's `software` branch. Preserve other remote branches. GitHub source updates do not deploy the separately hosted demo/API.
