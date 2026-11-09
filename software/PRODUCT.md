# BeatBlocks demo

<!-- impeccable:product-schema 1 -->

## Platform
web

## Users and purpose
Tianji and Ken can explore the physical rhythm-board concept and build a beat together from separate devices through one invitation link.

## Product facts
Two eight-step rows select kick and snare hits. A third web track adds hi-hat. LEDs show playback position. The course hardware remains a separate build.

## Demo scope
Web Audio playback, block editing, tempo, volume, presets, and local save/restore. Shared sessions persist kick, snare, hi-hat and tempo on the server. Both collaborators can edit all tracks. A continuous server timeline synchronizes playback position across devices. Play joins the current beat; Stop and volume remain local. Notes and tempo change at a shared scheduled loop boundary. Each browser synthesizes the sound and compensates for reported output timing.

## Implementation
GitHub Pages hosts the standalone frontend. The existing Sites project hosts an anonymous invitation-based HTTP API and D1 database. Clients poll every 600 ms after each response, send cell-level patches, retry pending changes, and ignore stale responses. Cryptographic invitation tokens grant editing access; no login or microphone is needed. No new paid account or parts are required.
