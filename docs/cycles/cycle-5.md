# Cycle 5 Report

Date completed: April 16, 2026

## Goal

Begin the world-layout phase by adding:

- the first reusable block descriptors for assembling more than one authored slice
- a second connected block extension beyond the current central intersection
- clearer boundaries between scene generation data and the renderer-facing box export
- the first spawn and interest-point hooks that future streaming and population systems can attach to

## What shipped

- `Sources/EngineCore` now builds the playable slice from reusable block descriptors instead of treating the whole scene as one monolithic handcrafted layout.
- The world now includes a second connected residential block north of the original hub intersection, which extends traversal into a longer two-block route.
- Scene generation now tracks authored block metadata and exported interest-point metadata separately from the renderer-facing box list, which gives the engine a cleaner seam for later chunking and streaming work.
- The engine now exports block descriptors and interest-point hooks for pedestrian spawn, vehicle spawn, landmarks and streaming anchors.
- `Sources/MusicToDriveBy` now reads the exported block and hook metadata so the overlay can report active layout state and nearest authored hook instead of only raw camera and actor state.
- The renderer ambient layer is now descriptor-driven, so intersection signals and local accent motion can follow authored block data rather than being hardcoded to the original intersection only.

## Notes on implementation

- The C engine scene build now runs through a small block layout table, dispatching block-specific frontage and landmark builders while preserving the existing box-export pipeline for rendering and collision.
- Ground-surface lookup now treats each authored block origin as part of the shared road corridor model, which keeps the second connected block traversable without introducing a new simulation path.
- Swift now copies exported block and interest-point arrays once at startup and uses them for overlay summaries and lightweight ambient placement.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Click inside the game view to capture mouse look and press `Esc` to release it.
- Walk north from the original hub intersection into the second block and confirm the route remains connected and readable in both first-person and third-person.
- Check the on-screen overlay and confirm it reports block count, hook count, the nearest authored block and the nearest interest hook as you move between the two intersections.
- Open `MusicToDriveBy.xcodeproj` and run the `MusicToDriveBy` scheme on `My Mac` if you want the Xcode app flow.

## Blockers and risks

- The block layout is still a tiny authored table rather than a scalable district or chunk graph, so streaming behavior is not in place yet.
- Interest points are exported and visible to the app, but no NPC, vehicle or encounter systems consume them yet.
- Rendering still relies on a fully expanded static box scene, which means later chunk activation and culling work will require another pass on scene ownership.
- The world reads as connected now, but content variety between blocks is still limited to graybox frontage changes and ambient accents.

## Next cycle goal

Cycle 6 should continue the world-layout foundation by adding:

- adjacency or road-graph data that makes block-to-block connections explicit instead of inferred
- the first active-block or nearby-block evaluation pass driven by player position
- early chunk-aware culling or activation hooks for props and authored metadata
- a first placeholder consumer for pedestrian and vehicle spawn hooks so the exported points drive visible systems instead of only debug state
