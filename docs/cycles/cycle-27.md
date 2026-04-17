# Cycle 27 Report

Date completed: April 17, 2026

## Goal

Convert the new territory-presence scaffold into the first authored faction actor behavior:

- replace at least one territory hold marker with a lightweight ambient faction body or patrol that can idle, watch, and hand pressure toward the lookout
- let territory presence bleed a little farther outside the lane mouth so the block starts influencing the adjoining sidewalk instead of only the combat pocket
- keep the UI restrained by preferring stronger world cues and tighter status text rather than adding more panels or debug-only reporting

## What shipped

- `Sources/EngineCore/include/engine_core.h` and `Sources/EngineCore/engine_core.c` now export a first court-set sidewalk patrol actor: position, heading, patrol state, and patrol alert. The new actor idles outside the lane mouth, shifts into a watch posture when the player brushes the edge, and steps inward into a handoff posture once pressure is moving deeper into the block.
- `Sources/EngineCore/engine_core.c` also feeds that patrol back into the territory rules. The sidewalk runner can hold some front-watch pressure outside the formal line, keep the adjoining sidewalk socially “warm” before the player fully commits, and add a small handoff bias to the lookout’s alert and attack thresholds once the player is inside claimed turf.
- `Sources/MusicToDriveBy/Renderer.swift` now renders that front presence as an actual ambient faction body instead of only a marker, while keeping the deeper hold as a simpler world cue. The result is a more legible social edge: one body on the sidewalk, one deeper hold inside the block, and a corridor cue once the handoff is live.
- `Sources/MusicToDriveBy/Renderer.swift` also adds a restrained text pass so the combat card, prompt pill, street card, and debug summaries can call out a watched sidewalk, a vehicle being read before the line, or the patrol handing pressure inward without adding new HUD panels.

## Notes on implementation

- The new patrol is intentionally lightweight. It is still a stateful placeholder body rather than a full population actor with pathfinding, dialogue, or combat of its own.
- The important behavioral change is that social pressure now starts before the player is technically “inside” the encounter strip. That gives the territory a more believable edge without needing a wider gang system yet.
- The deeper hold remains representational on purpose. One moving sidewalk actor plus one simpler inner hold keeps the slice readable while still letting the lookout feel supported instead of isolated.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk toward the lane mouth on foot and confirm a visible court-set sidewalk runner is now idling outside the line instead of leaving the territory edge empty.
- Step close enough to the lane mouth to flirt with the boundary and confirm the runner shifts into a watch or handoff posture while the prompt or street card calls out a watched sidewalk before full combat opens.
- Roll a staged vehicle along that same edge and confirm the runner plus street card now read the vehicle early with different language from the on-foot approach.
- Step deeper into the block and confirm the sidewalk runner hands pressure inward while the deeper hold marker and encounter readouts shift from the edge to claimed turf.
- Trigger the lookout, witnesses, and street incident as before, then confirm the older search, spillover, normalization, reopening, and hot re-entry states still read clearly with the new sidewalk actor active.

## Blockers and risks

- The edge of the territory now feels more alive, but the deeper hold is still a marker rather than a second ambient faction body.
- The sidewalk runner now biases the lookout a bit earlier, so the live feel will need tuning to stay tense without making the boundary feel unfair or overly sticky.
- The text remains restrained, but future faction work will need continued discipline so the prompt, street card, and encounter card do not all start repeating the same information.

## Next cycle goal

Cycle 28 should build on this first faction actor by adding a second lightweight handoff body inside the block:

- replace the deeper hold marker with a simple ambient faction body or inner post that can receive pressure from the sidewalk runner
- let the two faction bodies create a clearer edge-to-interior read without requiring full patrol AI or combat participation yet
- keep the UI pass narrow and lean harder on world staging than extra text so the territory remains readable at a glance
