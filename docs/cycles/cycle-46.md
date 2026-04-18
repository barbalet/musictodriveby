# Cycle 46 Report

Date completed: April 17, 2026

## Goal

Let the shoulder-biased reopening leave a clearer late fallback instead of disappearing as soon as recovery fades:

- let the last reopening shoulder bias which side the next neutral boundary watch and late fallback settle on after the explicit recovery timer ends
- keep that late fallback readable through the existing street card, prompt, lookout summary, and territory summary cues rather than adding a new system
- continue deepening the local faction-territory exchange before the broader mission and world-content frameworks arrive

## What shipped

- `Sources/EngineCore/engine_core.c` now keeps the patrol and inner-post cooldown settle slightly offset toward the remembered shoulder after reopening ends, so the block no longer drops straight back to a neutral center pose the moment street recovery expires.
- `Sources/EngineCore/engine_core.c` also now aims those late fallback heading targets through the same shoulder, which helps the next quiet line watch arrive from a softer left or right settle instead of from a centered reset.
- `Sources/MusicToDriveBy/Renderer.swift` now surfaces that late fallback through the existing prompt pill, street/system card, lookout summary, and territory summary. When the remembered shoulder is still materially active after reopening, those cues now say so instead of going silent until the next hot return.
- `README.md` now advances the repo to Cycle 47 and adds manual checks for same-shoulder late fallback and the next neutral boundary watch.

## Notes on implementation

- This pass still reuses the same `territory_preferred_side` memory. It does not add a separate post-recovery memory layer.
- The engine change is intentionally soft: cooldown positions and facing keep a same-shoulder settle while the remaining watch/heat carry is still strong enough, then fade out normally instead of snapping to center.
- The renderer continues to reuse the current HUD and summary surfaces. The new wording appears only when that fallback shoulder is actually still present.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Finish a same-side reopening reform, wait for the explicit street recovery timer to end, and confirm the runner/post pair now settle into a softer fallback on that same shoulder instead of dropping straight back to center.
- Begin the next quiet lane-mouth approach from that same side after recovery has already ended and confirm the first neutral watch still leans onto the remembered shoulder instead of reintroducing the line from a centered default.
- Watch the prompt, street/system card, lookout summary, and territory summary during that late fallback beat and confirm they all keep naming the same shoulder as a fallback or settle rather than going generic.
- Swap sides on a later loop and confirm the late fallback migrates with the new remembered shoulder instead of sticking to the previous one.

## Blockers and risks

- The late fallback still depends on the remaining watch and heat carry, so very soft or centered reopen outcomes can still cool back toward neutral quickly by design.
- This is still a pose-and-readout bias, not a richer long-tail patrol plan. The block feels more consistent after reopening, but it does not yet build a separate post-cooldown tactical pattern.
- Once the remaining watch carry fully drains, truly cold starts still collapse to the existing neutral boundary behavior.

## Next cycle goal

Cycle 47 should start letting that late fallback influence the first fully cold start instead of ending at the last soft settle:

- let the same remembered shoulder bias the earliest cold-start screen or clamp beat when the player returns after the fallback has nearly drained
- keep that cold-start carry readable through the existing street card, prompt, lookout summary, and territory summary cues rather than adding a new system
- continue deepening the local faction-territory exchange before the broader mission and world-content frameworks arrive
