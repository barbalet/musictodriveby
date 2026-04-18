# Cycle 47 Report

Date completed: April 17, 2026

## Goal

Let the shoulder-biased late fallback influence the first fully cold start instead of ending at the last soft settle:

- let the same remembered shoulder bias the earliest cold-start screen or clamp beat when the player returns after the fallback has nearly drained
- keep that cold-start carry readable through the existing street card, prompt, lookout summary, and territory summary cues rather than adding a new system
- continue deepening the local faction-territory exchange before the broader mission and world-content frameworks arrive

## What shipped

- `Sources/EngineCore/engine_core.c` now carries the remembered shoulder into the first cold outer-boundary screen, clamp, and early watch after late fallback has nearly drained, so the block no longer restarts that first quiet catch from a neutral center.
- `Sources/EngineCore/engine_core.c` also now leans lookout anchor choice toward that same shoulder during those first cold beats, which keeps the lookout read from snapping back to a generic edge while the runner/post pair still remember the prior side.
- `Sources/MusicToDriveBy/Renderer.swift` now surfaces that cold-start carry through the existing prompt pill, street/system card, lookout summary, and territory summary. When the first cold return is still shoulder-led, those cues now call it out as a cold carry instead of reading like a fully generic first contact.
- `README.md` now advances the repo to Cycle 48 and adds manual checks for same-shoulder cold-start screen and clamp behavior after late fallback nearly drains.

## Notes on implementation

- This pass still reuses the same `territory_preferred_side` memory. It does not add a separate long-idle memory or a new UI channel.
- The new carry is intentionally narrow: it only appears on the first quiet cold boundary beat while shoulder memory, watch carry, and low residual heat are still materially present.
- The lookout bias is lighter than the reclaim/retake reseal bias from earlier cycles. It is there to keep the readout coherent with the runner/post answer, not to fully override anchor choice in every cold approach.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Finish a same-side reopen sequence, let the late fallback almost drain, then re-approach from that same side and confirm the first cold screen now still forms on the remembered shoulder instead of from a neutral center.
- Repeat that pass with enough hesitation to force the first cold clamp and confirm that clamp also locks on the remembered shoulder instead of flattening back to a generic edge.
- Watch the prompt, street/system card, lookout summary, and territory summary during that first cold return and confirm they all call out the same shoulder as a cold carry rather than only showing generic first-contact wording.
- Try the same beat from a staged vehicle and confirm the cold shoulder carry survives there too instead of only showing up on foot.
- Swap sides on a later loop and confirm the next cold carry migrates with the new remembered shoulder instead of sticking to the previous side.

## Blockers and risks

- Once the preferred-side memory itself fully drains, truly neutral starts still resolve through the existing centered boundary logic by design.
- The new cold carry is stronger on the first screen or clamp than on a very soft watch-only read, so extremely shallow re-approaches can still feel partly neutral.
- This is still a positional and readout bias, not a richer long-tail patrol plan or a separate idle-state territory memory.

## Next cycle goal

Cycle 48 should start letting that remembered shoulder survive one beat earlier than the cold screen or clamp:

- let the same remembered shoulder bias the first fully neutral outside watch before the screen or clamp commits, so the lane read starts leaning on that side even sooner
- keep that softer pre-screen carry readable through the existing prompt, street card, lookout summary, and territory summary cues rather than adding a new system
- continue deepening the local faction-territory exchange before the broader mission and world-content frameworks arrive
