# Cycle 48 Report

Date completed: April 17, 2026

## Goal

Let the remembered shoulder survive one beat earlier than the cold screen or clamp:

- let the same remembered shoulder bias the first fully neutral outside watch before the screen or clamp commits, so the lane read starts leaning on that side even sooner
- keep that softer pre-screen carry readable through the existing prompt, street card, lookout summary, and territory summary cues rather than adding a new system
- continue deepening the local faction-territory exchange before the broader mission and world-content frameworks arrive

## What shipped

- `Sources/EngineCore/engine_core.c` now keeps the first neutral outside watch slightly leaned onto the remembered shoulder after late fallback and cold-start carry begin to fade, so the runner no longer restarts that first softer watch from a centered default.
- `Sources/EngineCore/engine_core.c` also now lets lookout anchor pressure follow that earlier outside-watch shoulder, which keeps the lookout read coherent with the runner instead of leaving the outer watch shoulder-biased while the lookout snaps back to a generic edge.
- `Sources/MusicToDriveBy/Renderer.swift` now surfaces that earlier beat through the existing prompt pill, street/system card, lookout summary, and territory summary. When the first outside watch is still shoulder-led, those cues now call it out as a watch carry instead of skipping straight from fallback to colder screen/clamp wording.
- `README.md` now advances the repo to Cycle 49 and adds manual checks for same-shoulder outside-watch carry ahead of the cold screen/clamp pass.

## Notes on implementation

- This pass still reuses the same `territory_preferred_side` memory and existing HUD surfaces. It does not introduce a new outer-watch state enum or a separate UI panel.
- The new outside-watch carry is softer than the cold screen/clamp carry from Cycle 47. It is meant to tilt the first neutral read, not to fully commit the block before the player earns the screen or clamp.
- The lookout pressure change is also intentionally light. It prefers the same shoulder when that earlier outside watch is active, but it does not override cover, travel, or line-of-sight constraints.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Finish a same-side reopen and late-fallback loop, then re-approach from that same side just far enough to trigger the first neutral outside watch without immediately forcing the screen. Confirm the runner now leans onto the remembered shoulder instead of reading you from a centered default.
- Hold that outside-watch distance for a moment and confirm the lookout anchor also favors that same shoulder instead of returning to a neutral edge choice while the runner is still shoulder-led.
- Step farther in from that same pass and confirm the colder screen or clamp still inherits the same shoulder instead of contradicting the earlier outside watch.
- Watch the prompt, street/system card, lookout summary, and territory summary during that first outside-watch beat and confirm they all call out the same shoulder as a watch carry before the colder screen/clamp language takes over.
- Repeat the pass from a staged vehicle and confirm the earlier outside-watch carry survives there too.
- Swap sides on a later loop and confirm the earlier outside-watch carry migrates with the new remembered shoulder instead of sticking to the previous one.

## Blockers and risks

- Once the preferred-side memory itself fully drains, the first outside watch still returns to the existing neutral read by design.
- The new outside-watch carry is intentionally softer than the screen/clamp carry, so very distant or very brief passes can still feel mostly neutral.
- This is still a pose, heading, and readout bias rather than a richer long-tail patrol route or a broader territory memory layer.

## Next cycle goal

Cycle 49 should start letting the remembered shoulder survive one beat earlier than the outside watch itself:

- let the same remembered shoulder bias the first far-out drift from idle into the runner’s outer post before the neutral outside watch fully forms
- keep that earlier approach carry readable through the existing prompt, street card, lookout summary, and territory summary cues rather than adding a new system
- continue deepening the local faction-territory exchange before the broader mission and world-content frameworks arrive
