# Cycle 25 Report

Date completed: April 17, 2026

## Goal

Begin the next phase by adding the first gang-territory encounter scaffolding:

- establish one lightweight faction boundary and one readable territory readout inside the current playable slice
- connect that territory to hostile spawn or pressure rules so the street starts to feel socially structured rather than mechanically generic
- add a first escalation rule that changes how the player is received when re-entering the hot block after provoking the slice
- keep the UI and world feedback restrained so new territory logic layers onto the stabilized Cycle 24 slice instead of overwhelming it

## What shipped

- `Sources/EngineCore/include/engine_core.h` and `Sources/EngineCore/engine_core.c` now export a first territory state layer for the combat slice: faction owner, territory phase, presence, heat, and hot re-entry timing. The current west-side lane now has an authored court-set boundary at its mouth, then a claimed block deeper inside the lane instead of behaving like socially neutral space.
- `Sources/EngineCore/engine_core.c` also feeds that territory state into hostile pressure. Inside claimed turf the lookout keeps a higher baseline alert and slightly longer reach, while a player who leaves after provoking the block and returns before it cools can trigger a hot re-entry that skips the old cold start and pushes the lookout into a faster reacquire/early-fire posture.
- `Sources/MusicToDriveBy/Renderer.swift` now reads territory state directly into the existing HUD and debug summaries. The street/system card can distinguish a territory line, claimed block, and burned territory, while the encounter card now identifies the lookout as part of that claimed space instead of as a generic lane target.
- `Sources/MusicToDriveBy/Renderer.swift` also adds territory-aware prompt language so the player can read the transition from neutral sidewalk to faction line to claimed turf without needing a separate debug-only concept of the block.

## Notes on implementation

- The faction layer is intentionally narrow. This is one authored territory band and one lookout-owned encounter block, not yet a general gang AI or district-ownership framework.
- Territory heat is driven by existing systemic signals like gunfire, hostile search, civilian reaction, and incident pressure. That keeps the scaffolding cheap and lets it inherit the same readable state transitions as the rest of the slice.
- Hot re-entry is the important new behavioral change: the block now remembers recent provocation long enough to alter how the player is received on return, which makes the slice feel socially authored instead of mechanically reset.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk north toward the lane and confirm the HUD or prompt now calls out a territory line before the block is fully hostile instead of leaving the lane socially neutral until the first shot.
- Step deeper into the block and confirm the encounter readout now identifies the lookout as a territory actor while the street card reports that you are inside claimed turf.
- Trigger the lookout, witnesses, and street incident as before, then confirm the underlying combat/civilian/traffic slice still reads cleanly through search, normalization, and reopening.
- Leave the block after provoking it, then re-enter before the territory cools and confirm the street card now reports a burned block while the lookout receives you faster instead of replaying the same cold approach.
- Check the debug combat and block summaries and confirm territory status, heat, and re-entry timing now appear alongside the existing street/system reporting instead of in a separate one-off overlay.

## Blockers and risks

- The territory scaffold is readable, but it is still one authored faction slice rather than a reusable multi-faction territory system.
- Hot re-entry now changes the feel of the block, but the exact timing and aggression still need live tuning so it feels tense rather than unfair.
- The HUD is still holding together, but future faction work will need careful restraint if territory, street pressure, and encounter detail are all going to stay legible together.

## Next cycle goal

Cycle 26 should build on the new faction scaffold by adding a first pass of visible faction presence:

- add one or two lightweight authored faction-presence markers or hold positions beyond the single lookout so the territory reads as occupied space instead of one actor plus a label
- connect those presence points to simple escalation or watch rules so crossing the territory line changes ambient reception before combat starts
- extend the territory readout so on-foot and vehicle entry communicate the difference between skimming the line and pushing deeper into claimed turf
- keep the implementation narrow and readable so faction presence strengthens the existing slice without breaking the clean combat/street-state layering from Cycles 24-25
