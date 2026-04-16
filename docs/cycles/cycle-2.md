# Cycle 2 Report

Date completed: April 16, 2026

## Goal

Deepen the traversal slice by adding:

- a broader connected set of reusable street segments
- stronger intersection and crossing readability
- better third-person presentation and follow behavior
- more street props and collision cases to validate movement feel

## What shipped

- The graybox slice is now a broader four-way street layout instead of a single straight corridor.
- The world includes a readable central intersection with crosswalks, stop bars and signal poles.
- Road, sidewalk and lot space now extend across a wider connected playable area.
- New solid traversal props were added, including planters, benches, parked cars, shelter geometry, poles and tree trunks.
- The outer lots now have more building clusters and prop density, which creates more real collision cases during movement.
- Third-person mode now uses a shoulder-follow camera with lead, occlusion-aware pull-in and smoother focus behavior.
- The actor is now rendered as a simple multi-part placeholder body with heading-based orientation instead of a single upright box.
- The debug overlay now includes actor heading so third-person presentation can be tuned more easily.

## Notes on implementation

- `Sources/EngineCore` now generates a wider cross-shaped road network, richer prop placement and denser collision geometry from reusable helpers.
- Surface classification now accounts for the combined north-south and east-west roads so traversal across the intersection remains coherent.
- `Sources/MusicToDriveBy` now renders a small articulated placeholder actor and uses the engine’s heading state for better third-person readability.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Toggle to third-person with `C` or `Tab`.
- Walk from a sidewalk through the main intersection and into the opposite lot to verify the wider connected slice.
- Open `MusicToDriveBy.xcodeproj` and run the `MusicToDriveBy` scheme on `My Mac` if you want the Xcode app flow.

## Blockers and risks

- The world is wider, but it is still a handcrafted graybox slice rather than a streamed district system.
- Collision remains box-based and does not yet support steps, ramps with bespoke rules, stairs or more advanced character movement.
- Third-person presentation is more readable, but it is still a procedural placeholder rather than an animation-driven character system.
- Input is still keyboard-first and would benefit from mouse look and controller support before longer traversal tuning passes.

## Next cycle goal

Cycle 3 should strengthen the traversal feel by adding:

- mouse look and cleaner camera/input ergonomics
- more landmark variation inside the current connected slice
- stronger curb, corner and obstruction feel tuning
- another pass on third-person follow and placeholder character readability
