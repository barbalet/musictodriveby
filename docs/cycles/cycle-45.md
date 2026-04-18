# Cycle 45 Report

Date completed: April 17, 2026

## Goal

Let the remembered shoulder start shaping where the block reforms during reopening instead of only how reopening is described:

- let reopening and post-reopen spillback start biasing which shoulder nearby traffic, sidewalk pressure, and boundary reform gather around
- keep that reform readable through the existing street card, prompt, lookout summary, and territory summary cues rather than adding a new system
- continue deepening the local faction-territory exchange before the broader mission and world-content frameworks arrive

## What shipped

- `Sources/EngineCore/engine_core.c` now lets street-recovery reopening pull the runner/post reform back toward the remembered shoulder outside the line instead of reforming from a more neutral center every time. The boundary can now reopen through the same left or right shoulder that the previous reclaim or reseal favored.
- `Sources/EngineCore/engine_core.c` also now adds a shoulder-biased reopening spillback occupancy, so nearby recovery traffic/link pressure can gather around that same side instead of reopening the block as one uniform pulse.
- `Sources/MusicToDriveBy/Renderer.swift` now calls that shoulder-led reopening reform out through the existing prompt pill, combat HUD, and street/system card while keeping the already-added lookout and territory summary shoulder tags intact.
- `README.md` now advances the repo to Cycle 46 and adds manual checks for shoulder-led reopening reform and spillback on foot and in vehicles.

## Notes on implementation

- This pass still reuses the same `territory_preferred_side` memory and the same street recovery layer. It does not add a separate reopening planner.
- The boundary reform change lives in the existing outer territory branch: reopening can now select a reform posture that leans the runner and inner post back toward the remembered shoulder before the next hot return.
- Traffic spillback remains lightweight and occupancy-driven. The new behavior comes from adding a shoulder-biased reopening spill position and link pulse rather than from a more detailed traffic simulation.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Finish a same-side held-pocket reclaim or same-side clean pullout, let the lane normalize into reopening, then hover back near the mouth from that same side. Confirm the runner/post reform now gathers around that shoulder instead of reforming from a neutral center.
- Repeat that reopen pass in a staged vehicle and confirm the prompt plus combat HUD now call out the same shoulder-led reform instead of falling back to generic vehicle reform language.
- Watch the street/system card during that reopen beat and confirm it now describes the same shoulder reopening and reforming there rather than only a whole-block reopening message.
- Swap sides on a later loop and confirm the reopening reform and spillback migrate with the new remembered shoulder instead of staying pinned to the earlier one.

## Blockers and risks

- The reopening reform still depends on the remembered shoulder being strong enough to matter, so centered exits or centered reopen passes can still read intentionally neutral.
- Traffic spillback is still occupancy-based rather than route-planned, so the new shoulder bias reads as a stronger reopening pulse rather than as a fully authored lane-by-lane return pattern.
- The new reform branch is confined to the reopening window; once that recovery layer fades, later cold-start boundary reads still rely on the existing territory watch behavior.

## Next cycle goal

Cycle 46 should start letting the shoulder-biased reopening leave a clearer late fallback instead of disappearing as soon as recovery fades:

- let the last reopening shoulder bias which side the next neutral boundary watch and late fallback settle on after the explicit recovery timer ends
- keep that late fallback readable through the existing street card, prompt, lookout summary, and territory summary cues rather than adding a new system
- continue deepening the local faction-territory exchange before the broader mission and world-content frameworks arrive
