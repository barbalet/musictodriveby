# Cycle 13 Report

Date completed: April 17, 2026

## Goal

Continue the vehicle-fundamentals phase by adding:

- stronger player-facing handoff selection so nearby staged vehicles are easier to rank and identify in dense moments
- engine-side traffic occupancy hooks so placeholder traffic can start respecting shared lanes and stop or hold zones more systemically
- more distinct bicycle and motorcycle feel, including clearer camera posture and class-specific motion cues
- early near-miss or hazard feedback so traffic pressure is readable before it becomes a full collision or damage pass

## What shipped

- `Sources/EngineCore/include/engine_core.h` and `Sources/EngineCore/engine_core.c` now expose ranked nearby-vehicle state and exported traffic occupancy records. The engine tracks a primary, secondary, and tertiary staged handoff candidate plus dynamic occupancy zones for staged vehicles, the active player vehicle, and on-road pedestrian crossing pressure.
- Handoff selection is no longer just "nearest thing wins." `Sources/MusicToDriveBy/Renderer.swift` now uses the engine-ranked candidate set to drive clearer prompts, ranked debug summaries, and stronger world markers so dense curbside moments are easier to read before the player presses `F`.
- Ambient placeholder traffic now respects engine-owned occupancy hooks instead of a renderer-only blocker list. Traffic samples hold short or drop out around staged vehicles, the active player vehicle, and the player on the road using the exported occupancy strength and radius data.
- Bicycle and motorcycle presentation now has stronger class identity. The engine feeds steer-lean state into the camera pass so bicycles feel lighter and more upright while motorcycles feel lower, faster, and more aggressive in both first-person and third-person framing.
- Early traffic hazard feedback now appears in-world and in the debug overlay. When ambient traffic pressure closes in on the player or active vehicle, a pulsing hazard marker and summary line make the near-miss state readable before the project has a full collision-damage or law-response pass.

## Notes on implementation

- The new occupancy layer is a hook, not a full traffic simulation. It exports hold zones and pressure records the renderer can consume, but there is still no lane queue, right-of-way solver, or authored stop-sign logic.
- Ranked handoff selection is still passive. The player does not yet cycle through candidates manually, but the current best three options are now surfaced coherently in the prompt, overlay, and marker stack.
- Bicycle and motorcycle feel are still graybox, but the camera posture, lean, and bob cues now make those classes read as more than just narrower cars.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk into an area where multiple staged vehicles are nearby and confirm the prompt now prefers the strongest ranked candidate while still showing the next-best option.
- Check the debug window and confirm the new `handoff` and `hazard` lines update as you approach staged vehicles and moving placeholder traffic.
- Stand on or near a road link and confirm ambient traffic now yields against the engine-exported occupancy zones instead of only reacting to renderer-local blocker positions.
- Ride the bicycle and motorcycle in both first-person and third-person and confirm they now show stronger class-specific camera posture and motion cues than the sedan, coupe, and moped.
- Move into the path of placeholder traffic and confirm the early hazard pulse appears before any full collision or damage model exists.

## Blockers and risks

- Traffic still does not reason about intersections, signals, stop order, or lane ownership as true agents.
- Ranked handoff selection is clearer now, but the player still cannot manually rotate through candidates when several are tightly packed.
- Hazard feedback is readable, but it is still presentation-only and not tied to injuries, missions, law response, or systemic vehicle damage.
- Bicycle and motorcycle feel are more distinct, but there is still no rider animation, lean model, mount transition, or per-class audio.

## Next cycle goal

Cycle 14 should close the vehicle-fundamentals phase by adding:

- lane-hold and stop-zone rules that make ambient traffic pause more deliberately at shared chokepoints and intersections
- manual handoff candidate stepping or lock confirmation so dense curbside clusters remain predictable under player input
- tighter bicycle and motorcycle edge-case polish, especially around curb hops, low-speed turning, and dismount placement
- a final vehicle-fundamentals cleanup pass that resolves obvious roster inconsistencies before the project moves into combat foundations
