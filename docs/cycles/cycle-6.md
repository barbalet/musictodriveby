# Cycle 6 Report

Date completed: April 16, 2026

## Goal

Continue the world-layout foundation by adding:

- adjacency or road-graph data that makes block-to-block connections explicit instead of inferred
- the first active-block or nearby-block evaluation pass driven by player position
- early chunk-aware culling or activation hooks for props and authored metadata
- a first placeholder consumer for pedestrian and vehicle spawn hooks so the exported points drive visible systems instead of only debug state

## What shipped

- `Sources/EngineCore` now exports an explicit road link between the two authored blocks instead of leaving that connection implied by layout order alone.
- Block descriptors now carry activation radius data, and interest-point hooks now remember which authored block they belong to.
- The engine runtime now evaluates the active block, nearby block count, active road link, and active pedestrian and vehicle spawn counts from player position each frame.
- `Sources/MusicToDriveBy` now reads the exported road graph and runtime activity state so the overlay can report live graph and activation information instead of only static layout counts.
- The renderer now culls its dynamic authored metadata layer to active and nearby blocks, which creates the first chunk-aware activation pass for ambient accents and hook visuals.
- Pedestrian and vehicle spawn hooks now drive visible placeholder population in the nearby blocks, so the hook data is feeding rendered world behavior rather than existing only as exported metadata.

## Notes on implementation

- The road graph currently consists of a single north-south link, but it now exists as authored export data with its own copy API and a runtime “active link” concept in the engine state.
- Dynamic activation is currently limited to the overlay and renderer-side dynamic props, not the underlying static graybox box scene, so future chunk ownership work will still need to move deeper into scene generation and render submission.
- Placeholder pedestrians and vehicles are render-only consumers of spawn hooks; they do not yet participate in collision, AI, or traffic rules.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Click inside the game view to capture mouse look and press `Esc` to release it.
- Walk between the two intersections and confirm the overlay updates the active block, nearby count, active link, and active pedestrian and vehicle hook counts as you move.
- Verify that placeholder pedestrians and vehicles appear around nearby spawn hooks and fade from relevance when you move away from their owning block.
- Open `MusicToDriveBy.xcodeproj` and run the `MusicToDriveBy` scheme on `My Mac` if you want the Xcode app flow.

## Blockers and risks

- The scene still renders from one expanded static box export, so activation currently affects only dynamic metadata and placeholder population rather than full world geometry.
- Road graph data is explicit now, but it is still tiny and hand-authored rather than generated from a larger district layout.
- Placeholder spawn consumers make the hook system visible, but there is still no true pedestrian simulation, traffic logic, or spawn/despawn lifecycle management.
- Active-block evaluation is strong enough for this slice, but future streaming work will need broader chunk identifiers, ownership boundaries, and memory-management rules.

## Next cycle goal

Cycle 7 should build on these foundations by adding:

- a third connected authored block or lane branch so the graph stops being purely linear
- first lightweight spawn lifecycle rules so placeholder population can appear and disappear more intentionally
- a deeper split between static world geometry and activation-driven dynamic scene content
- early authoring conventions for district identity or block tags that later systems can use for missions, gangs, and population flavor
