# Cycle 12 Report

Date completed: April 17, 2026

## Goal

Continue the vehicle-fundamentals phase by adding:

- bicycle and motorcycle handoff states so the early vehicle roster better matches the intended fantasy
- first ambient traffic occupancy rules so the player vehicle and placeholder traffic stop feeling fully non-interactive
- stronger impact and recovery feedback, including clearer collision consequences and camera response
- more deliberate handoff presentation, such as cleaner mounting context, vehicle-specific prompts, and better staging readability at distance

## What shipped

- `Sources/EngineCore/include/engine_core.h` and `Sources/EngineCore/engine_core.c` now support two additional playable vehicle classes, `bicycle` and `motorcycle`, with their own tuning for speed, steering, collision radius, seating, exit spacing, and camera framing.
- The staged handoff set now covers sedan, coupe, moped, bicycle, and motorcycle across the four-block slice. Residential frontage now stages a bicycle alongside the coupe pass, and the service-spur block now stages a motorcycle alongside the service-side coupe.
- Ambient placeholder traffic no longer treats the world as fully empty. `Sources/MusicToDriveBy/Renderer.swift` now applies first occupancy-yield rules around staged vehicles and the active player vehicle, which causes ambient traffic to hold short or drop out instead of visually ghosting through occupied space.
- Vehicle impacts now carry a short recovery state rather than a one-frame bump only. Collision and off-road jolts feed a recovery value that briefly reduces composure, softens lane assist, dampens acceleration, and drives stronger first-person and third-person camera response.
- Handoff presentation is clearer at distance. Staged vehicles now render with persistent kind-colored signal markers, richer vehicle-specific prompts, and class-specific placeholder silhouettes so bicycles and motorcycles read differently from the existing sedan, coupe, and moped set.

## Notes on implementation

- The new occupancy rules are still renderer-side placeholder logic. They do not yet reserve lanes in the engine or simulate ambient traffic decisions as systemic world actors.
- Bicycle and motorcycle handling are still graybox placeholders, but they now differ materially from the car and moped set in top speed, steering agility, camera spacing, and dismount feel.
- Recovery is intentionally short and readable. It is meant to make curb hits and wall impacts legible without turning the current prototype into a damage-model pass.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk the slice and confirm the staged roster now includes bicycle and motorcycle handoffs in addition to the existing sedan, coupe, and moped set.
- Mount each class with `F` and verify the overlay reports the correct vehicle type, plus grip, lane, bump, and recovery values while moving.
- Drive or ride into walls, curbs, sidewalks, and lots and confirm the collision feedback now lingers briefly as a recovery state with stronger camera response than Cycle 11.
- Stay on the active roads while driving and confirm ambient placeholder traffic now yields or clears around staged vehicles and the active player vehicle instead of overlapping them as freely as before.
- Approach staged vehicles from a distance and confirm the new marker colors and silhouettes make it easier to pick out each handoff class before reaching prompt range.
- Use both first-person and third-person views with bicycle and motorcycle and confirm their framing feels different from the sedan, coupe, and moped passes.

## Blockers and risks

- Ambient occupancy is still a presentation rule rather than a simulation rule. Traffic does not yet stop at authored controls, queue in lanes, or resolve right-of-way.
- Bicycle and motorcycle classes still use placeholder geometry, no rider animation, and no lean model.
- Recovery feedback is more legible now, but there is still no persistent vehicle damage, no destruction state, and no vehicle-to-vehicle impact resolution.
- The current handoff prompts are clearer, but there is still no diegetic HUD or screen-space guidance for locating the broader vehicle roster.

## Next cycle goal

Cycle 13 should continue the vehicle-fundamentals phase by adding:

- stronger player-facing handoff selection so nearby staged vehicles are easier to rank and identify in dense moments
- engine-side traffic occupancy hooks so placeholder traffic can start respecting shared lanes and stop or hold zones more systemically
- more distinct bicycle and motorcycle feel, including clearer camera posture and class-specific motion cues
- early near-miss or hazard feedback so traffic pressure is readable before it becomes a full collision or damage pass
