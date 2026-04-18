# Cycle 49 Report

Date completed: April 17, 2026

## Goal

Let the remembered shoulder survive one beat earlier than the outside watch itself:

- let the same remembered shoulder bias the first far-out drift from idle into the runner’s outer post before the neutral outside watch fully forms
- keep that earlier approach carry readable through the existing prompt, street card, lookout summary, and territory summary cues rather than adding a new system
- continue deepening the local faction-territory exchange before the broader mission and world-content frameworks arrive

## What shipped

- `Sources/EngineCore/engine_core.c` now lets the runner drift off its idle base into a far-out outer-post approach on the remembered shoulder before the later outside-watch beat fully forms, so the block no longer waits until the closer watch pass to stop feeling centered.
- `Sources/EngineCore/engine_core.c` also now preserves a lighter front-watch carry during that far-out approach and lets lookout anchor pressure follow it, which keeps the lookout read coherent with the runner instead of leaving the earliest drift shoulder-led while the lookout stays generic.
- `Sources/MusicToDriveBy/Renderer.swift` now surfaces that earliest beat through the existing prompt pill, street/system card, lookout summary, and territory summary. When the runner is still only drifting into the remembered shoulder, those cues now call it out as an approach carry instead of skipping straight to outside-watch language.
- `README.md` now advances the repo to Cycle 50 and adds manual checks for same-shoulder outer-post approach carry ahead of the outside-watch pass.

## Notes on implementation

- This pass still reuses the same `territory_preferred_side` memory and the existing patrol watch state. It does not add a separate pre-watch patrol enum or a new HUD panel.
- The new outer-post carry is intentionally softer than the outside-watch carry from Cycle 48. It is meant to wake the runner early, not to fully commit the lane before the closer watch has formed.
- The earlier front-watch carry is also intentionally lighter than the later watch carry, so very distant passes can still cool back toward neutral faster than the closer boundary beats.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Finish a same-side late-fallback and cold-return loop, then begin the next approach from that same side far enough out that the runner has only just started drifting off its base. Confirm that earliest drift now leans onto the remembered shoulder instead of waiting until the later outside watch to stop feeling centered.
- Hold that farther-out distance briefly and confirm the lookout anchor also starts favoring that same shoulder instead of remaining generic while the runner is already drifting there.
- Ease farther in from that same pass and confirm the later outside watch still inherits the same shoulder instead of contradicting the earlier drift.
- Watch the prompt, street/system card, lookout summary, and territory summary during that earliest drift and confirm they all call out the same shoulder as an approach carry before the outside-watch wording takes over.
- Repeat the pass from a staged vehicle and confirm the earlier approach carry survives there too.
- Swap sides on a later loop and confirm the earliest outer-post drift migrates with the new remembered shoulder instead of sticking to the previous one.

## Blockers and risks

- Once the preferred-side memory itself fully drains, the runner still leaves idle through the existing neutral path by design.
- The new outer-post carry is intentionally subtle, so very brief or very wide passes can still read as only a small lean instead of a dramatic reposition.
- This is still a positional, heading, and readout bias rather than a richer far-perimeter patrol route or a broader long-tail territory memory layer.

## Next cycle goal

Cycle 50 should start letting the remembered shoulder survive one beat earlier than that first outer-post drift:

- let the same remembered shoulder bias the first far-perimeter facing or idle lean before the runner visibly leaves its base
- keep that earliest pre-drift carry readable through the existing prompt, street card, lookout summary, and territory summary cues rather than adding a new system
- continue deepening the local faction-territory exchange before the broader mission and world-content frameworks arrive
