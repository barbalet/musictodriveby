# Cycle 97 Report

Date completed: April 18, 2026

## Goal

Keep refining the shared node-control kit:

- target another reusable mast or signal-corner nuance around a small signal-arm collar edge breakup or a subtle mast-wiring clamp seam hint so the controlled crossings keep gaining realism without bespoke props
- continue building from shared road-spine and intersection metadata rather than bespoke one-off node geometry
- preserve the many-mile Los Angeles path by keeping the next detail layer reusable across the widened slice

## What shipped

- `Sources/EngineCore/engine_core.c` now expands the shared signal-arm reinforcement path with a small collar-edge breakup: darker collar-edge bands plus a tiny top cap across the reinforcement collar stack.
- The new collar-edge helper is called from the same shared mast assembly already used by every signalized head cluster, so the added breakup rides across the node kit without bespoke placement.
- Signal poles now read less like smooth collar blocks around the arm reinforcement and more like maintained fabricated supports with a little collar edge structure.
- `README.md` now advances the repo to Cycle 98, adds manual checks for the new signal-arm collar pass, and updates the prototype summary with the added fabricated-collar read.

## Notes on implementation

- The collar-edge bands stay narrow and dark so they read as fabricated edge breakup rather than decorative trim.
- A tiny top cap helps the collar cluster feel terminated and assembled instead of leaving the new edge bands floating as isolated dark marks.
- Because the helper keys off `arm_sign`, the signal-arm collar detail follows arm orientation instead of reading like one pasted world-space strip.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Look at the signal-arm collar blocks on several poles and confirm the new edge breakup reads as fabricated collar detail instead of smooth support blocks around the arm reinforcement.
- Compare the darker collar-edge bands against the tiny top cap on the same collar cluster and confirm the new breakup stays subtle rather than turning into a bold trim stripe.
- Compare opposite-facing mast heads and confirm the signal-arm collar detail follows the arm orientation instead of reading like one pasted world-space strip.
- Compare several districts and confirm the signal-arm collar pass still reads as a shared scalable kit rather than a bespoke hero-node treatment.

## Blockers and risks

- The new signal-arm collar pass improves the reinforcement read, but it still relies on shallow box geometry and color breakup rather than true collar seams, bolts, or textured fabricated hardware.
- As the project expands toward much broader Los Angeles coverage, the collar-edge contrast and cap size may need retuning so the new breakup stays readable without becoming noisy at different distances.
- The project still needs much larger-scale geographic and visual realism than the current sixteen-block prototype, so this pass should remain reusable procedural foundation work instead of a local polish cul-de-sac.

## Next cycle goal

Cycle 98 should keep refining the shared node-control kit:

- target another reusable mast or signal-corner nuance around a subtle mast-wiring clamp seam hint or a small arm-span cable-drop collar breakup so the controlled crossings keep gaining realism without bespoke props
- continue building from shared road-spine and intersection metadata rather than bespoke one-off node geometry
- preserve the many-mile Los Angeles path by keeping the next detail layer reusable across the widened slice
