# Cycle 52 Report

Date completed: April 17, 2026

## Goal

Move the widened Los Angeles reference grid past painted hierarchy and into a more believable street-section pass:

- give boulevards and avenues different physical cross-sections instead of only different markings
- make the road, curb, sidewalk, ground query, and vehicle lane pull all read from the same road-class metadata
- keep the current graybox stable while pushing corridor geometry closer to an import-ready descriptor model

## What shipped

- `Sources/EngineCore/engine_core.c` now defines shared `MDTBRoadProfile` descriptors for boulevard, avenue, connector, and residential street classes, including road width, curb outer edge, sidewalk depth, lane offset, and boulevard median width.
- `Sources/EngineCore/engine_core.c` now uses those profiles to build the visible road surface pass. Boulevards render broader asphalt, slightly wider curb reveal, and deeper sidewalk carry than avenues, so the corridor type is readable from the geometry before lane paint or HUD text does the work.
- `Sources/EngineCore/engine_core.c` now feeds the same road profiles into road features, markings, ground classification, and vehicle lane assist. That means curb ramps, sidewalk reach, and lane pull no longer assume one universal street width underneath every corridor.
- `README.md` now advances the repo to Cycle 53, updates the prototype summary to mention the new road-class cross-section layer, and adds manual checks for cross-section feel, curb transition feel, and lane-pull differences between boulevard and avenue corridors.

## Notes on implementation

- This is still an authored graybox road model, not imported Los Angeles GIS or lane-count data. The improvement is consistency and shape differentiation, not final real-world geometry.
- The new profile layer is intentionally compact so future corridor import work can replace the current hardcoded values without rewriting every consumer again.
- Ground classification now follows the nearest road profile, which keeps curb and sidewalk transitions aligned with visible geometry but still assumes a simple nearest-corridor model at intersections.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk from one east-west boulevard onto a north-south avenue and confirm the street changes section before markings: boulevard carriageways should feel broader and avenues should tighten up sooner at the curb.
- Cross from asphalt to curb to sidewalk on foot and on a bicycle or motorcycle and confirm the height or grip transition follows the active corridor type instead of one reused curb envelope.
- Drive a sedan or coupe down both corridor types and confirm lane assist settles slightly wider on boulevards and slightly tighter on avenues.
- Re-enter the original combat lane and confirm the existing territory, hostile, civilian, and traffic behavior still reads correctly inside the updated street geometry.

## Blockers and risks

- The cross-sections are still authored per road class, so they improve plausibility but do not yet reflect parcel-specific frontage, turn pockets, alleys, or real imported lane topology.
- Intersections are still shared graybox junctions, so the new corridor widths help the straightaways more than they help node complexity.
- Vehicle logic now respects road-class lane offsets, but it is still a simplified lane-centering model rather than full multi-lane traffic behavior.

## Next cycle goal

Cycle 53 should keep pushing the world toward a production street skeleton:

- separate corridor and tile descriptor data further from one large engine source blob so the widened map is easier to scale
- start adding more intersection-aware geometry and block-edge shaping so the widened corridors do not all terminate in the same junction kit
- continue moving the larger Los Angeles slice toward a many-mile-ready foundation instead of a polished but still clearly authored sixteen-block testbed
