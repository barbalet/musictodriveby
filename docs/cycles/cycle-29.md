# Cycle 29 Report

Date completed: April 17, 2026

## Goal

Make the paired territory bodies feel more coordinated without broadening the system:

- add a simple paired fallback routine so the sidewalk runner and inner post settle and reset in a more readable sequence after search, hot re-entry, or cooling
- let the inner post shift a little more around the claimed pocket so the two bodies feel staged rather than fixed on rails
- keep the UI pass minimal and prefer stronger world motion plus a few better encounter phrases over any new panels or debug-heavy text

## What shipped

- `Sources/EngineCore/include/engine_core.h` and `Sources/EngineCore/engine_core.c` now carry full exported state for the deeper inner post, including position, heading, state, and alert, so the second court-set body can be driven as a real paired actor instead of a fixed placeholder marker.
- `Sources/EngineCore/engine_core.c` now stages a clearer runner-to-post sequence. The inner post wakes earlier during deeper pushes, hot territory returns, and provoked states, drifts toward a receive pocket instead of one rigid point, and then cools back on a slight delay so the sidewalk runner can settle first while the deeper body lingers for a beat.
- `Sources/EngineCore/engine_core.c` also threads that inner-post support into lookout pressure more deliberately. Inner-post alert and live handoff now contribute a small amount to territory heat, deep watch carry, hostile alert floors, attack range, and claimed-territory attack thresholds, which makes hot re-entry and deeper pushes read as coordinated pressure rather than isolated markers.
- `Sources/MusicToDriveBy/Renderer.swift` now renders the deeper body directly from the moving engine state, keeps the corridor cue attached to the live runner/post positions, and tightens the encounter, prompt, street-card, and debug language so the UI only calls out the inner-post handoff when it is actually active.

## Notes on implementation

- The coordination remains intentionally lightweight. This is still a staged social-presence pass, not full NPC behavior with general locomotion, combat roles, or dialogue.
- The key change is timing. The runner and inner post no longer wake and reset as a single blocky pulse; they now read as a small paired setup with a handoff, a lingering deeper presence, and a cleaner stagger back toward idle.
- The visual motion is doing most of the work. The text pass stays narrow and mostly reinforces the moments where the deeper body is actually taking over pressure.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk toward the lane mouth on foot and confirm the court-set sidewalk runner still idles and watches outside the line as before.
- Step deeper into claimed turf and confirm the runner now hands pressure toward the inner post while the deeper body shifts a little within its pocket instead of locking to one exact marker.
- Back off after the handoff is live and confirm the sidewalk runner settles first while the inner post lingers deeper for a moment before both bodies finish cooling.
- Re-enter while the block is still hot and confirm the runner/post pair recover in a readable sequence instead of snapping back together into the same posture.
- Trigger the lookout, civilians, and street incident as before, then confirm the older search, spillover, normalization, reopening, and hot re-entry behavior still reads clearly with the staggered runner/post fallback in place.

## Blockers and risks

- The pair now reads more like a coordinated faction setup, but both bodies are still lightweight authored presences rather than general-purpose gang actors.
- The inner post contributes a bit more to lookout pressure now, so live tuning is still needed to keep the block tense without making hot territory feel overly stacked.
- The deeper body moves more naturally, but the territory still depends on a small number of authored pockets and timings rather than broader street-level faction logic.

## Next cycle goal

Cycle 30 should add one more small layer of authored territory pressure without turning these actors into full AI:

- give the sidewalk edge a more readable intimidation beat so the runner can briefly challenge or screen the entry before full combat pressure starts
- let the runner and inner post use a few more staged offsets or stance changes so repeated approaches do not always silhouette the same way
- keep the lookout as the real combat threat and preserve the lean HUD approach by putting most of the new value into world motion and timing
