# Cycle 75 Report

Date completed: April 18, 2026

## Goal

Keep refining the shared node-control kit:

- target another reusable mast or signal-corner nuance around pedestrian-signal readout hardware or secondary mast wiring detail so the controlled crossings keep gaining realism without bespoke props
- continue building from shared road-spine and intersection metadata rather than bespoke one-off node geometry
- preserve the many-mile Los Angeles path by keeping the next detail layer reusable across the widened slice

## What shipped

- `Sources/EngineCore/engine_core.c` now expands the shared pole-corner path with paired pedestrian-signal readouts aligned to the inward crossing directions on each signalized corner.
- The existing shared corner helper still places that new readout hardware from the same inward-facing pole logic already used for the call boxes, so every node inherits the upgrade without bespoke authoring.
- Signalized corners now read less like button hardware alone and more like controlled crossings with visible pedestrian display hardware.
- `README.md` now advances the repo to Cycle 76, adds manual checks for the new pedestrian-readout pass, and updates the prototype summary to call out the paired crossing displays.

## Notes on implementation

- The new readout helper sits alongside the existing pole-corner hardware instead of replacing it, which keeps the call-box layer and the pedestrian display layer separated for future tuning.
- Each corner now gets two small readout housings aligned to the two inward crossing directions, using axis-aware dimensions so the housings rotate with the crossing they serve.
- The displays use separate upper and lower readout bands plus a rear mount block, which helps them read as compact pedestrian signals instead of flat add-on slabs.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Stand beside the inward-facing sides of several signal poles and confirm each corner now carries small paired pedestrian-signal readouts above the call boxes instead of stopping at button hardware alone.
- Compare the x-facing and z-facing readouts on the same corner and confirm the new housings rotate with the crossing direction instead of reading like one pasted display slab.
- Look for the amber upper readout and lighter lower readout on those same housings and confirm the display break-up stays visible without overpowering the existing pole hardware.
- Compare several districts and confirm the new pedestrian-readout layer still reads like a shared scalable kit rather than bespoke node dressing.

## Blockers and risks

- The new readout pass improves the crossing hardware, but it still relies on stepped box geometry and color breakup rather than authored pedestrian-signal meshes, lit materials, or animated walk/don't-walk states.
- As the road network expands toward much broader Los Angeles coverage, the readout size and mount offsets may need tuning so they stay believable across a wider range of future corner conditions.
- The project still needs much larger-scale geographic and visual realism than the current sixteen-block prototype, so this pass should remain a reusable procedural foundation instead of a local polish cul-de-sac.

## Next cycle goal

Cycle 76 should keep refining the shared node-control kit:

- target another reusable mast or signal-corner nuance around secondary mast wiring detail or lighter signal-face weathering so the controlled crossings keep gaining realism without bespoke props
- continue building from shared road-spine and intersection metadata rather than bespoke one-off node geometry
- preserve the many-mile Los Angeles path by keeping the next detail layer reusable across the widened slice
