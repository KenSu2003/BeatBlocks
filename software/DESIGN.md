# BeatBlocks demo design

The physical board leads. A warm neutral tabletop holds a dark wood-toned enclosure, recessed empty slots, and raised cream blocks with blue kick and orange snare identifiers. Controls look like instrument controls. Copy is short and concrete.

Colors: background #eeeae3, ink #252c30, muted #596267, panel #292e30, slot #202526, block #f1ece0, kick #92c8da, snare #f1ad82, action #195f73. Typography uses Georgia for the compact display title and system sans for labels. Main content max width 1120px. Eight steps remain a single row on phones. All controls retain visible focus; reduced motion removes token transitions.

Only the instrument and hi-hat panel are enclosed surfaces. The hi-hat panel stays collapsed in solo mode and opens in shared sessions. Local saves are labeled separately from session autosave.

## Shared sessions
Keep the board as the focus. A compact strip above it holds Create session, invitation link, connection state and Leave session. Joining opens the hi-hat panel and enables direct shared editing. Play joins the shared clock at its current position. Stop, volume and hi-hat mute control only the current device. The board displays the current synchronized position; pending edits join at a shared loop boundary. Clearly distinguish session autosave from browser-local saved copies.
