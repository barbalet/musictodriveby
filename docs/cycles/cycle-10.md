# Cycle 10 Report

Date completed: April 16, 2026

## Goal

Start the vehicle-fundamentals phase by adding:

- first player-facing vehicle mount or handoff hooks at selected authored vehicle points
- an initial vehicle control state for at least one class so the player can move from on-foot traversal into a drivable placeholder
- first-person and third-person vehicle camera behavior that preserves the current traversal presentation standards
- reuse of chunk, hotspot, and block metadata to choose which parked vehicles are interactable and where early vehicle handoffs are staged

## What shipped

- `Sources/EngineCore/include/engine_core.h` and `Sources/EngineCore/engine_core.c` now export staged vehicle anchors, traversal-mode state, and player vehicle runtime data so the prototype can treat a sedan handoff as authored world state instead of a renderer-only trick.
- Each authored frontage template now stages a sedan handoff point on the current four-block slice, which gives both west and east chunks a reachable drivable vehicle while still keeping the handoff placement driven by block and frontage metadata.
- The engine now supports entering a staged sedan with `F`, placeholder driving and braking with the existing movement controls, low-speed exit back to on-foot traversal, and runtime anchor updates so the staged car keeps following the player across blocks and chunks after it has been moved.
- Vehicle camera behavior now has dedicated first-person and third-person branches. First-person sits inside the sedan with constrained look offsets, while third-person uses a wider chase framing than the on-foot camera.
- `Sources/MusicToDriveBy/Renderer.swift`, `Sources/MusicToDriveBy/InputController.swift`, and `Sources/MusicToDriveBy/GameViewModel.swift` now render staged parked sedans with mount and active highlights, surface vehicle-specific overlay status and prompts, and document the new `F` interaction in the controls panel.

## Notes on implementation

- This is still a placeholder driving model. The sedan currently uses simple authored anchor handoff, acceleration, drag, steering, and scene-box collision rather than full wheel, suspension, or vehicle-to-vehicle simulation.
- Vehicle anchors are mutable runtime state now. Once the player drives a staged sedan somewhere else and exits, that sedan stays at its new location because the engine updates the anchor export rather than respawning it at the authored start point.
- The current slice only exposes one playable vehicle class, sedan, even though the broader milestone phase still includes bicycle, motorcycle, moped, and car states.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Spawn near the west-grid sedan, walk up to it, and confirm the overlay prompts `F` to enter.
- Enter the sedan, drive through the west chunk, and confirm the overlay switches to vehicle status plus first-person or third-person vehicle camera labels.
- Toggle camera mode while driving and check that the first-person view stays cabin-adjacent while the third-person view widens into a chase camera.
- Slow down, press `F` to exit, and confirm the sedan remains parked at its new location instead of snapping back to its authored staging point.
- Cross between west and east chunks while driving and confirm the staged sedan remains visible as block-owned geometry streams around the active chunk set.

## Blockers and risks

- Vehicle handling is intentionally lightweight and still ignores proper tire slip, curb response, vehicle damage, and collisions against other moving traffic.
- The player can now enter and exit a sedan cleanly, but bicycles, motorcycles, mopeds, and differentiated car classes are not in yet.
- Staged vehicle selection is metadata-driven, but the authoring vocabulary is still small and limited to the current four-block slice.
- The UI prompt and debug overlay are useful for discovery, but there is not yet a diegetic prompt, minimap cue, or mission consumer for vehicle handoff points.

## Next cycle goal

Cycle 11 should deepen the vehicle-fundamentals pass by adding:

- a second and third playable vehicle class so the handoff flow is no longer sedan-only
- better placeholder vehicle collision and curb or surface response so driving off the road has readable consequences
- per-vehicle tuning for acceleration, turning radius, camera distance, and exit points
- early traffic-lane or parking-state rules so staged and ambient vehicles start to feel like part of the same system
