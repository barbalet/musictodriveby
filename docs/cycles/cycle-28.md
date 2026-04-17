# Cycle 28 Report

Date completed: April 17, 2026

## Goal

Build on the first faction actor by adding a second lightweight handoff body inside the block:

- replace the deeper hold marker with a simple ambient faction body or inner post that can receive pressure from the sidewalk runner
- let the two faction bodies create a clearer edge-to-interior read without requiring full patrol AI or combat participation yet
- keep the UI pass narrow and lean harder on world staging than extra text so the territory remains readable at a glance

## What shipped

- `Sources/EngineCore/include/engine_core.h` and `Sources/EngineCore/engine_core.c` now export a second court-set territory actor: an inner post with position, heading, state, and alert. The inner post idles deeper in claimed turf, wakes when the player pushes in or the sidewalk runner hands pressure inward, and cools back to its base when the block settles.
- `Sources/EngineCore/engine_core.c` also ties that inner post into the existing territory watch loop. The runner now feeds pressure forward into the inner post instead of only inflating a generic deep-watch number, and the inner post adds a smaller but readable contribution to deep watch, territory heat, lookout alerting, and attack thresholds when the handoff is live.
- `Sources/MusicToDriveBy/Renderer.swift` now renders the inner post from engine state instead of as a fixed deeper marker. The block now reads as two lightweight faction bodies with a corridor cue between them: one body on the sidewalk edge and one body deeper inside the lane.
- `Sources/MusicToDriveBy/Renderer.swift` also tightens the text pass so claimed-territory prompts, encounter details, street-card messages, and debug summaries can name the inner post when it is actually active, while keeping the sidewalk-watch branch and the existing search/incident/reopening language intact.

## Notes on implementation

- The inner post is still intentionally light. It is not yet a full NPC with pathfinding, combat, or ambient chatter; it is a second stateful placeholder body that strengthens the social read of the block.
- The important change is relational rather than additive. The sidewalk runner now has somewhere to hand pressure, so the territory reads as a paired edge-to-interior setup instead of one actor plus one abstract deeper cue.
- The UI changes stay narrow on purpose. The stronger world staging does most of the work, and the text only switches to runner/post wording when the inner actor is actually awake.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk toward the lane mouth on foot and confirm the court-set sidewalk runner still idles and watches outside the line as before.
- Look deeper into the lane and confirm the old deeper hold marker is now a second ambient court-set body instead of a fixed marker.
- Step deeper into claimed turf and confirm the runner now hands pressure toward that inner post while the encounter readout shifts to runner/post language once the handoff is active.
- Roll a staged vehicle through the edge and into claimed turf and confirm the street card or prompt now distinguishes the early sidewalk read from the later inner-post handoff.
- Trigger the lookout, witnesses, and street incident as before, then confirm the older search, spillover, normalization, reopening, and hot re-entry behavior still reads clearly with both faction bodies present.

## Blockers and risks

- The territory now has a clearer paired setup, but both bodies are still lightweight placeholders rather than general population actors with richer behavior.
- The lookout now gets a little more support from the inner post, so live tuning is still needed to keep the claimed turf tense without making the slice feel stacked or unfair.
- The text remains restrained, but future faction work will need to keep leaning on world motion and staging instead of turning runner/post state into another verbose HUD layer.

## Next cycle goal

Cycle 29 should make the paired territory bodies feel more coordinated without broadening the system:

- add a simple paired fallback routine so the sidewalk runner and inner post settle and reset in a more readable sequence after search, hot re-entry, or cooling
- let the inner post shift a little more around the claimed pocket so the two bodies feel staged rather than fixed on rails
- keep the UI pass minimal and prefer stronger world motion plus a few better encounter phrases over any new panels or debug-heavy text
