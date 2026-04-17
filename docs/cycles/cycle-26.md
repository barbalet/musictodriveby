# Cycle 26 Report

Date completed: April 17, 2026

## Goal

Build on the first territory scaffold by adding a narrow pass of visible faction presence:

- add one or two lightweight authored faction-presence markers or hold positions beyond the single lookout so the territory reads as occupied space instead of one actor plus a label
- connect those hold points to simple watch rules so crossing the line changes ambient reception before combat opens
- extend the territory readout so on-foot and vehicle entry communicate the difference between skimming the line and pushing deeper into claimed turf
- keep the implementation narrow and readable so the new presence layer strengthens the stabilized slice instead of overwhelming it

## What shipped

- `Sources/EngineCore/include/engine_core.h` and `Sources/EngineCore/engine_core.c` now export a lightweight territory-watch layer on top of the earlier faction boundary work. The engine tracks entry mode, front-hold pressure, deep-hold pressure, and a short watch timer so the block can read the difference between brushing the lane mouth and pushing farther into claimed turf.
- `Sources/EngineCore/engine_core.c` also feeds that watch state back into hostile pressure. Crossing the line in a vehicle can keep the front hold hotter at the mouth, while deeper entry pushes more weight into the inner hold and lowers the threshold for the lookout to settle into a faster response inside claimed territory.
- `Sources/MusicToDriveBy/Renderer.swift` now renders two simple authored court-set hold markers in-world, plus a faint corridor cue when the deeper hold is active. That makes the territory read as occupied social space before the actual exchange starts instead of relying only on labels in the HUD.
- `Sources/MusicToDriveBy/Renderer.swift` also updates the combat card, street card, and prompt text so on-foot entry, vehicle skimming, deeper pushes, and hot re-entry each get distinct language. The HUD can now call out a territory line, claimed turf, or a vehicle being clocked at the mouth without waiting for the combat state machine to fully light up.

## Notes on implementation

- The new presence layer is intentionally representational. The front and deep holds are authored watch anchors and world markers, not yet full faction NPCs with patrol logic.
- Entry mode matters now because it changes reception. A vehicle skim keeps the line hot in a different way than an on-foot advance, which gives the territory more texture without needing a larger AI system yet.
- The watch state stays tied to the existing territory, search, and hot re-entry framework so the block still feels like one coherent slice rather than a separate faction minigame layered on top.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk toward the lane mouth on foot and confirm the street card or prompt calls out the territory line while a visible front-hold marker makes the edge of the block feel occupied before gunfire starts.
- Roll a staged vehicle across that same boundary and confirm the prompt now describes the vehicle as being clocked at the line instead of using the same readout as on-foot entry.
- Step deeper into claimed turf and confirm the deeper hold marker and encounter readout now make the block feel watched in layers rather than by only one lookout.
- Trigger the lookout, witnesses, and street incident as before, then confirm the existing search, spillover, normalization, reopening, and hot re-entry states still read clearly with the new hold/watch layer active.
- Leave after provoking the block, then re-enter before it cools and confirm the hot-territory behavior still picks you up fast while the new front/deep hold language remains readable.

## Blockers and risks

- The territory now reads as occupied space, but those holds are still markers and watch values rather than full faction actors.
- Vehicle-versus-on-foot reception is clearer, but it will need live tuning so the added distinction feels authored rather than just more text.
- Future faction work will need discipline so additional presence, patrol, or encounter logic does not overcrowd the HUD and street-state messaging that is currently staying readable.

## Next cycle goal

Cycle 27 should convert this presence scaffold into the first authored faction actor behavior:

- replace at least one territory hold marker with a lightweight ambient faction body or patrol that can idle, watch, and hand pressure toward the lookout
- let territory presence bleed a little farther outside the lane mouth so the block starts influencing the adjoining sidewalk instead of only the combat pocket
- keep the UI restrained by preferring stronger world cues and tighter status text rather than adding more panels or debug-only reporting
