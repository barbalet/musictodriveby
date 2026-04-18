# Cycle 51 Report

Date completed: April 17, 2026

## Goal

Turn the widened Los Angeles reference grid into a more road-authentic foundation:

- differentiate boulevards, avenues, and service-lane frontage so the sixteen-block world no longer reads like one repeated street kit
- move road generation a step closer to import-ready data by driving more of the engine from shared road metadata instead of duplicated block-grid assumptions
- keep the named corridor layer readable while improving the visible street hierarchy in the actual world

## What shipped

- `Sources/EngineCore/engine_core.c` now builds shared road-spine descriptors from the named road-link metadata and uses them to drive world surfaces, lane markings, and ground queries instead of relying on repeated unique-`x` and unique-`z` scans over the block layout.
- `Sources/EngineCore/engine_core.c` also now gives boulevards and avenues distinct visual treatment. Boulevards carry a stronger center median and broader intersection read, while avenues now show curb-lane bands and boundary striping so the north-south corridors stop looking identical to the east-west arterials.
- `Sources/EngineCore/engine_core.c` further strengthens the service-spur frontage with dedicated loading-zone paint and curbside staging, so those mixed-use spur blocks start reading like service-road pockets instead of generic curb frontage.
- `README.md` now advances the repo to Cycle 52, updates the prototype summary to mention the shared road-spine layer and road hierarchy pass, and adds manual checks for boulevard, avenue, and service-lane differentiation.

## Notes on implementation

- This pass still keeps the same graybox traffic and collision widths. The new hierarchy is primarily a visual and metadata cleanup pass, not yet a full lane-count or curb-geometry rewrite.
- The new `g_road_spines` layer is intentionally small and derived from the existing named road links. That keeps the current prototype stable while moving more of the engine toward a reusable road-descriptor model the future import pipeline can replace.
- Service-lane treatment is still localized to the existing spur frontage blocks. It is a readability pass, not yet a broader alley or back-of-house network simulation.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk or drive across the east-west corridors and confirm the boulevards now show a clearer center median and broader stop-bar or arrow spacing than the north-south avenues.
- Move up one of the north-south corridors and confirm the avenues now show curb-lane bands and side boundary striping instead of the same center treatment as the boulevards.
- Step into one of the service-spur mixed-use blocks and confirm the frontage now shows clearer loading-zone paint and curbside staging.
- Re-enter the original combat lane and confirm the territory, hostile, and spillover behavior still reads correctly inside the upgraded wider street network.

## Blockers and risks

- The street hierarchy is still a graybox surface pass, so it improves readability but does not yet deliver realistic lane counts, medians, gutters, or curb geometry.
- The shared road-spine model is derived from authored code data today; it is cleaner than before, but it is not yet consuming imported Los Angeles source data.
- Because traffic and handling still use the existing simplified lane logic, vehicles can benefit from the clearer street read visually before they fully respect a richer future road-spec model.

## Next cycle goal

Cycle 52 should start pushing the geometry itself toward the next realism bar:

- use the road-spine layer to begin differentiating corridor cross-sections, curb shapes, and lane-space allocation instead of limiting hierarchy to overlays and markings
- keep moving the world model toward importable descriptors by separating corridor or tile data from one large authored layout blob
- continue improving the larger-map street read so the widened world feels closer to a believable Los Angeles road skeleton and less like a polished graybox
