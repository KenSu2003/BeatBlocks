# Selectable Sound Sets Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement this plan task by task.

**Goal:** Switch the same two-row beat among Drums, Percussion, and Wood on the browser and ESP32.
**Architecture:** A shared catalog identifies sets; matching PCM assets feed independent browser and board players. Room JSON and scheduled playback frames carry the selected set. The existing USB control path selects the board's set.
**Tech Stack:** JavaScript, Web Audio, existing HTTP/D1 API, Arduino ESP32-S3 C++, I²S, Web Serial.
**Spec:** `../specs/2026-11-16-sound-sets.md`

## Global constraints

- Sets: `drums` = Kick/Snare, `percussion` = Tom/Clap, `wood` = Woodblock/Claves.
- Two rows, eight steps, 50–170 BPM; existing pins and hi-hat behavior stay intact.
- Legacy beats default to drums. Unsupported IDs fail validation.
- New sets use matching preloaded PCM on firmware and browser; preserve each existing Drums playback path. No new hardware or dependencies.
- Board selection uses USB. Cloud sessions synchronize browsers.
- Publish the source to branch `software`; no automatic board flashing or remote-branch merge.

## Review focus

- Existing rooms and local saves must still load.
- A queued/optimistic set must not play before its committed boundary.
- Concurrent note and set changes must both survive retries.
- Long sample tails must not suppress subsequent hits at 170 BPM.
- USB disconnect or invalid input must not change the active set or stop the sequencer.

## Task 1 — Catalog and shared-session state

Files: `software/lib/sound-catalog.mjs`, `lib/sessions.mjs`, `lib/timing.mjs`, `lib/session-client.mjs`, and their tests.
Interfaces: `SOUND_SETS`, `DEFAULT_SOUND_SET`, `isSoundSet(id)`; room/frame field `soundSet`; edit operation `{soundSet: id}`.

- [x] Add failing tests for default/invalid IDs, save/rejoin, concurrent edits, retry deduplication, queued overlays, and boundary selection.
- [x] Run `node --test tests/*.test.mjs` and confirm the new cases fail.
- [x] Implement validation, legacy defaults, client overlays, and timeline state; allow 26 operations for restoring 24 cells plus tempo and set.
- [x] Run the suite and commit the session changes.

## Task 2 — Matching samples and browser controls

Files: `software/scripts/generate-sound-sets.mjs`, `lib/sound-samples.mjs`, `public/index.html`, `scripts/sync-demo.mjs`, and browser/audio tests; generated new firmware PCM headers.
Interfaces: generated `SOUND_PCM` keyed by instrument, sample rate 44100; browser `playInstrument(id, time)`.

- [x] Add failing tests for selecting/labeling sets without changing notes, save/restore, and sound choice at shared/solo boundaries.
- [x] Generate four new bounded PCM sounds; preserve existing drum samples and export all six to the browser.
- [x] Add the sound-set selector, dynamic labels, matching AudioBuffers, local persistence, and shared edits.
- [x] Re-embed modules with `node scripts/sync-demo.mjs`; run the full suite and build; commit.

## Task 3 — Physical board and USB selector

Files: `sound/reed_bridge/SoundBank.h`, `SampleMixer.h`, `reed_bridge.ino`, `sound/reed_sim/index.html`, `sound/tests/`.
Interfaces: fixed-size polyphonic mixer; `KIT:drums`, `KIT:percussion`, `KIT:wood`; `OK queued KIT:<id>`, `OK live KIT:<id>`; `Q` returns `KIT:<id>`.

- [x] Add failing native tests for instrument mapping, invalid set commands, deferred boundary switches, overlapping tails, and bounded mixing.
- [x] Implement the generic sample bank/mixer and a bounded cross-core hit queue; preserve pin assignments and reed/USB OR behavior.
- [x] Add USB selector and active/pending feedback, with connection errors handled clearly.
- [x] Run native tests and ESP32 compilation; record memory usage. Commit.

## Task 4 — End-to-end verification and delivery

- [x] Run all browser/server/native tests and the production web build.
- [x] Exercise the local D1 API with two clients, set changes, save/rejoin, and invalid IDs.
- [x] Inspect the browser selector, row labels, local restore, and shared-room flow.
- [x] Add concise setup and board acceptance steps; record hardware checks still requiring the board.
- [x] Review the diff against `ddeeb11` and fix the two serial edge cases with failing-then-passing regression tests.

Delivery: push `software` and verify that its remote commit matches the tested local commit.

Bench acceptance: flash once; connect USB; build a physical rhythm; switch all three sets without moving blocks; verify activation at step zero; run dense patterns at 50, 96, and 170 BPM; disconnect USB and verify continued playback. Confirm both browser clients hear the chosen set after the same shared boundary.
