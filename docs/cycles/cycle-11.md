# Cycle 11 Report

Date completed: April 16, 2026

## Goal

Deepen the vehicle-fundamentals pass by adding:

- a second and third playable vehicle class so the handoff flow is no longer sedan-only
- better placeholder vehicle collision and curb or surface response so driving off the road has readable consequences
- per-vehicle tuning for acceleration, turning radius, camera distance, and exit points
- early traffic-lane or parking-state rules so staged and ambient vehicles start to feel like part of the same system

## What shipped

- `Sources/EngineCore/include/engine_core.h` and `Sources/EngineCore/engine_core.c` now expose richer vehicle-anchor data, including parking state, lane axis, and lane offset, plus runtime grip, lane-error, and collision-pulse values for the active player vehicle.
- The staged handoff set is no longer sedan-only. The current four-block slice now stages sedan, coupe, and moped handoffs across the authored frontage templates, which makes the vehicle loop materially broader without increasing map size.
- The engine now uses per-vehicle tuning for acceleration, braking, drag, steering, collision radius, seating position, camera limits, and exit spacing, so the sedan, coupe, and moped feel meaningfully different instead of sharing one generic handling table.
- Placeholder driving now has stronger surface consequences and early lane guidance. Road links pull the player vehicle toward a lane center and a matching heading, while curbs, sidewalks, and lots cut grip, raise bump intensity, and make off-road driving feel less stable.
- `Sources/MusicToDriveBy/Renderer.swift` now renders staged and ambient vehicles through the same class-aware placeholder path, adds curbside or service parking guides for staged handoffs, and surfaces grip, lane, bump, and parking-state feedback in the overlay and prompts.

## Notes on implementation

- The lane system is intentionally light-touch. It is a steering assist and lane-centering rule built from the authored road-link graph rather than a full traffic, occupancy, or avoidance model.
- Parking state is authored metadata for now. It currently changes staging visuals, prompts, and debug state rather than driving tickets, despawn rules, or mission logic.
- The coupe and moped still use placeholder geometry and the same button mapping as the sedan, but their tuning, seating, camera framing, and exit spacing now differ.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk the slice and confirm different blocks now stage different vehicle classes, including a moped and at least one coupe.
- Mount each class with `F` and verify that the overlay reports the correct vehicle type plus grip, lane, and bump values.
- Drive onto curbs, sidewalks, or lots and confirm the vehicle loses composure relative to the road, with more readable off-road consequences than the Cycle 10 pass.
- Stay on the main roads and confirm the vehicle now settles back toward a lane and a road-aligned heading instead of drifting as freely as before.
- Toggle between first-person and third-person while driving each class and confirm the camera distance and seating feel different across sedan, coupe, and moped.
- Exit after moving a vehicle and confirm the staged handoff remains at the new position with its updated parking state still visible.

## Blockers and risks

- Lane guidance is still assistive rather than systemic. Ambient traffic does not yet stop, yield, avoid the player, or reason about occupied lanes.
- Vehicle collision is more legible now, but there is still no damage model, no vehicle-to-vehicle impact resolution, and no destruction state.
- The current class spread covers sedan, coupe, and moped only. Bicycle and motorcycle-specific handling and presentation are still missing.
- Parking-state metadata is now present, but it is still mostly a renderer and debug consumer rather than a gameplay system.

## Next cycle goal

Cycle 12 should continue the vehicle-fundamentals phase by adding:

- bicycle and motorcycle handoff states so the early vehicle roster better matches the intended fantasy
- first ambient traffic occupancy rules so the player vehicle and placeholder traffic stop feeling fully non-interactive
- stronger impact and recovery feedback, including clearer collision consequences and camera response
- more deliberate handoff presentation, such as cleaner mounting context, vehicle-specific prompts, and better staging readability at distance
