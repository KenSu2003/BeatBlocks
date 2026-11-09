# BeatBlocks

Live demo: https://hitianji.github.io/beatblocks-demo/

Click **Create session**, copy the invitation link, and open it on another device. Both people can edit the kick, snare, hi-hat, and tempo. Press **Play** whenever you join: playback follows the session's current position. Session changes save automatically; **Save local copy** keeps a separate browser-local copy.

## Development

Requires **Node.js 22.13 or newer** and npm. From the repository root:

```sh
cd software
npm run install:ci
npm run build
node --import ./scripts/sites-env.mjs ./node_modules/wrangler/bin/wrangler.js d1 execute DB --config dist/server/wrangler.json --local --persist-to .wrangler/state --file drizzle/0000_premium_madame_web.sql
npm start
```

Open the local URL printed by Wrangler (normally `http://127.0.0.1:8787`). Run the SQL command once for a new local database. This runs the frontend and shared-session API together, using local Cloudflare D1 storage; no Cloudflare sign-in is needed for local testing. Two browser windows at the same address can join a session through its invitation link.

```sh
node --test tests/*.test.mjs
```

- Frontend: `public/index.html`, a standalone HTML/CSS/JS file.
- Network client: `lib/session-client.mjs`; shared clock and timeline: `lib/timing.mjs`. Run `node scripts/sync-demo.mjs` to embed updates into the HTML.
- API: `lib/sessions.mjs`, exposed by `app/api/sessions` route handlers.
- Database: D1 schema in `db/schema.ts`; immutable migrations in `drizzle/`.
- `node --test tests/*.test.mjs` checks concurrent edits, retries, persistence, input validation, CORS, late playback starts, output latency mapping, tempo changes, and reconnects.
- `npm run dev` provides the Vite development preview. The build/start commands above are the reproducible local setup.

GitHub Pages serves only `index.html` and its README. The Sites deployment supplies the API and a mirrored demo. No server credentials are included in the frontend.

The existing live demo uses `https://beatblocks-playground.homerclare.chatgpt.site` for its session API. A new deployment needs a Cloudflare D1 database bound as `DB`, the included SQL migration, and an API endpoint. If serving the frontend on a separate origin, update `API_BASE` in `public/index.html` and the allowed origins in `lib/sessions.mjs`. Uploading this folder to GitHub does not deploy that backend.

## Session behavior

A 256-bit invitation token grants read/write access. No room listing or accounts. GET requests fetch authoritative state; POST edits contain absolute cell values, client sequence numbers and a batch of changes. Compare-and-swap retries preserve concurrent edits to different cells. For the same cell, the last server-accepted change wins. Sequence receipts prevent retried operations from replaying over newer changes. A browser queues unacknowledged edits until reconnect; reloading with pending edits prompts before discarding them. Leaving a session keeps the current beat for solo play.

The server supplies one continuous musical timeline. Play joins its current position; Stop, volume, and hi-hat mute affect only the current device. Notes and tempo activate together at a server-selected loop boundary at least 1.8 seconds after acceptance. Optimistic edits update the controls immediately; audio uses the committed timeline. Clients estimate server time using monotonic clocks and minimum-delay timestamp samples, refreshed while polling. Web Audio schedules notes ahead and compensates for browser-reported output timing. Network asymmetry and unreported speaker/Bluetooth delay still affect acoustic alignment.

After four seconds without room updates, playback waits for reconnection and rejoins the current position. A hidden tab stops playback; returning users press Play to rejoin. Heartbeats run every 10 seconds; presence expires after 35 seconds. Rooms persist in D1. Rate limits allow 50 session creations per IP/day and 600 edit requests per room/minute. Sessions accept up to 256 distinct client instances; a reload creates a new client instance.
