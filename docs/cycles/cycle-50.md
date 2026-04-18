# Cycle 50 Report

Date completed: April 17, 2026

## Goal

Start the post-alpha realism reset with a real Los Angeles reference foundation:

- widen the graybox from the original four-block sandbox into a named district grid that can support many-mile planning
- add named corridor, road-class, district, and tile metadata that the engine and HUD can both read
- surface that world context in-app so the new geo direction is visible during normal play rather than only in docs

## What shipped

- `Sources/EngineCore/engine_core.c` now expands the playable reference layout from the original four-block slice into a sixteen-block Los Angeles-style grid with four streamed tiles, wider world bounds, and a larger named road skeleton.
- `Sources/EngineCore/include/engine_core.h` and `Sources/EngineCore/engine_core.c` now carry named district, tile, road-class, and corridor enums plus richer `MDTBRoadLink` metadata, so the world can identify arterial corridors such as Crenshaw Blvd, Jefferson Blvd, Exposition Blvd, Western Ave, and Vermont Ave instead of only generic east-west or north-south links.
- `Sources/EngineCore/engine_core.c` also generalizes the older hub and residential frontage builders so their authored props, landmarks, and spawn hooks stay local to each block origin instead of collapsing back onto the original `x=0` strip when the map grows.
- `Sources/MusicToDriveBy/Renderer.swift`, `Sources/MusicToDriveBy/GameViewModel.swift`, and `Sources/MusicToDriveBy/GameRootView.swift` now surface that geo layer through the debug summary and a new in-scene HUD map card that reports the active district, corridor, road class, and streaming tile while keeping the larger static road skeleton visible on screen.
- `README.md` now advances the repo to Cycle 51, updates the prototype summary to reflect the wider named LA grid, and adds manual checks for district and corridor readouts.

## Notes on implementation

- This cycle still uses authored graybox geometry. The new named grid is a geo-foundation pass, not yet a parcel-accurate or art-finished Los Angeles build.
- The old combat lane and territory systems still live in the original southwest slice on purpose, so the wider world can grow around a stable gameplay anchor while the new data model comes online.
- Static world rendering now keeps the broader reference grid visible, while ambient simulation remains more local so the prototype can widen coverage without immediately overcommitting traffic and pedestrian budgets everywhere.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk or drive north and east out of the original lane sandbox and confirm the world now extends into a sixteen-block named reference grid instead of ending after the original four-block slice.
- Watch the HUD map card and floating controls window while crossing roads and confirm they report named districts, tiles, and corridors such as West Adams, Jefferson Park, Crenshaw Blvd, Exposition Blvd, Western Ave, or Vermont Ave.
- Pull the camera back in third person and confirm the wider corridor skeleton stays visible at once instead of chunking back to only the immediate combat strip.
- Re-enter the original combat lane and confirm the territory, hostile, witness, and traffic systems still behave as before inside the widened world.

## Blockers and risks

- The widened map is still assembled from reused graybox block kits, so it now reads as a larger city skeleton but not yet as a professional-quality realistic street build.
- Corridor names and district names are currently authored directly in code; the data model is richer now, but the import pipeline is not yet pulling from external Los Angeles source data.
- Ambient simulation still favors local visibility and authored hooks, so long-distance traffic and pedestrian density are not yet representative of the final many-mile target.

## Next cycle goal

Cycle 51 should start turning this named grid into a more road-authentic foundation:

- use the new road-class and corridor metadata to differentiate boulevard, avenue, and service-road presentation instead of drawing every corridor the same way
- keep hardening the geo layer toward import-ready tile data rather than leaving the widened layout as one monolithic authored array
- improve the visible street read so the widened map feels less like repeated blockout strips and more like the first believable Los Angeles road pass
