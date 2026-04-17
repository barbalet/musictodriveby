# Cycle 9 Report

Date completed: April 16, 2026

## Goal

Build on the Cycle 8 grid and ownership pass by adding:

- first block-family or frontage-template variation rules so new authored blocks stop relying on near-duplicate hand layout
- link-aware placeholder movement across the four-block graph so pedestrians and vehicles start to read like they belong to routes instead of isolated spawn loops
- stronger chunk or ownership identifiers that move the current render-side cull toward true streaming boundaries
- another consumer for authored population profiles or block identity data, ideally a first mission, gang-territory, or hotspot hook

## What shipped

- `Sources/EngineCore/include/engine_core.h` and `Sources/EngineCore/engine_core.c` now export frontage-template ids and chunk ids on each authored block, which gives the layout a first reusable vocabulary for block-family variation and chunk-scale ownership.
- The world now uses two authored streaming chunks, west and east, and `Sources/MusicToDriveBy/Renderer.swift` culls block-owned static scene content by active chunk instead of only by directly active block, which is a clearer step toward real streaming boundaries.
- Frontage authoring now routes through template selection, and the northeast Market Spur block uses a distinct service-spur frontage instead of sharing the same mixed-use frontage layout as the earlier east block.
- The engine now authors hotspot hooks per frontage template, and the renderer surfaces those hooks as visible markers plus overlay counts, giving block identity and population metadata a new consumer beyond placeholder spawn timing and tinting.
- Placeholder pedestrians and vehicles now build route samples from the authored road-link graph, with vehicles able to chain across multiple links and pedestrians steering toward linked hotspots and landmarks instead of staying in local orbit loops.

## Notes on implementation

- Chunking is still intentionally coarse. The current slice groups the two west blocks together and the two east blocks together so the renderer can start thinking in chunk boundaries without needing a full streaming system yet.
- Frontage templates currently cover the authored four-block slice rather than a broad content library, but the block builders now resolve frontage from metadata instead of assuming one hardcoded layout per district or kind.
- Hotspots are authoring hooks only for now. They currently feed overlay/debug state, visible world markers, and placeholder routing targets rather than missions, gangs, or scripted activities.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk around the west chunk and then cross east into the Market Spur chunk.
- Confirm the overlay reports chunk labels, visible block counts, hotspot counts, and frontage-template metadata for the active block.
- Verify that moving east brings the east chunk’s owned buildings and frontage online together while the west chunk falls back to the shared road grid.
- Watch placeholder pedestrians and vehicles near intersections and confirm they now travel along authored links and spill between linked blocks instead of only circling their local spawn anchors.
- Check that hotspot markers appear at district-relevant corners such as the hub retail frontage, Maple Heights apartment and bus-stop edge, and the northeast service spur.

## Blockers and risks

- Chunk ownership is still renderer-driven visibility, not true streamed memory ownership or async load and unload behavior.
- Route-aware placeholder movement now reads better, but it still ignores collision, traffic rules, lane discipline, and pedestrian avoidance.
- Frontage templates are a useful seam now, but the library is still tiny and the underlying geometry is still hand-authored.
- Hotspots exist as authored hooks and movement targets only; they are not yet connected to missions, gangs, or any true systemic encounter logic.

## Next cycle goal

Cycle 10 should start the vehicle-fundamentals phase by adding:

- first player-facing vehicle mount or handoff hooks at selected authored vehicle points
- an initial vehicle control state for at least one class so the player can move from on-foot traversal into a drivable placeholder
- first-person and third-person vehicle camera behavior that preserves the current traversal presentation standards
- reuse of chunk, hotspot, and block metadata to choose which parked vehicles are interactable and where early vehicle handoffs are staged
