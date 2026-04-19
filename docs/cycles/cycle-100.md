# Cycle 100 Report

Date completed: April 18, 2026

## Goal

Make Cycle 100 a real Los Angeles map step instead of another tiny prop-detail pass:

- expand the engine from the old sixteen-block reference slice into a meaningfully broader LA-style road skeleton
- carry named corridor, district, chunk, and population metadata across that widened footprint so the app can reason about much more of the gang-map target
- preserve the existing authored Crenshaw-to-Vermont combat slice while widening the map around it instead of breaking the current playable core

## What shipped

- `Sources/EngineCore/include/engine_core.h` now defines a much broader Los Angeles naming set with sixteen districts, nine streaming tiles, and sixteen named corridors instead of the older small-core enum set.
- `Sources/EngineCore/engine_core.c` now grows the world capacities, widens the playable bounds, and replaces the tiny hand-authored block or road tables with an eight-by-eight layout grid that preserves the old core while extending west, north, east, and south.
- The road skeleton is now generated from shared corridor tables instead of a short static link list, so the engine exports a full sixty-four-block, sixteen-corridor, cross-city network with generated road links and generated road spines.
- Population profiles are now generated from widened district, tag, and chunk metadata instead of being locked to the old sixteen-block table, which gives the expanded map coherent travel and density reads without bespoke per-block authoring.
- `Sources/MusicToDriveBy/Renderer.swift` now understands the larger district, tile, and corridor set so the map card, block summary, activity summary, and ambient tinting all read correctly across the widened LA footprint.
- `README.md` now advances the repo to Cycle 101, adds manual checks for the enlarged LA skeleton, and updates the prototype summary to describe the broader connected world instead of the old sixteen-block slice.

## Notes on implementation

- The widened layout keeps the older `0/96/192/288` and `0/72/144/216` corridor coordinates intact inside the larger grid, so the authored combat lane still sits on the same named core roads while the world expands around it.
- Road links now come from grid-aware generation keyed off shared row and column corridor metadata, which keeps the larger map scalable and avoids turning the new LA skeleton into another fragile hand-maintained link list.
- Chunk assignment remains procedural as well, so the world can expose broader tile labels and chunk-sensitive frontage or population behavior without requiring one-off placement for every new block.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Open the map card or debug overlay and confirm the engine now reports sixty-four blocks, nine tiles, and sixteen named roads instead of the older sixteen-block slice.
- Travel west-to-east and confirm corridor readouts now hand off through Fairfax, La Brea, Crenshaw, Arlington, Western, Vermont, Figueroa, and Central.
- Travel south and confirm corridor readouts now hand off through Pico, Washington, Adams, Jefferson, Exposition, Martin Luther King Jr, Slauson, and Florence.
- Push out toward the far west, far east, and deep south edges and confirm blocks, staging, road markings, and district or tile labels still render coherently instead of dropping into empty out-of-bounds space.

## Blockers and risks

- The enlarged LA map pass is still a structured road skeleton, not a faithful full parcel or freeway reconstruction of the linked gang map, so some districts still read as repeated block grammar rather than distinct neighborhoods.
- Outer tiles now exist in the engine and HUD, but they still need more district-specific frontage, traffic, encounter, and landmark logic so the far edges do not feel like copied core fabric.
- The larger world increases scene, collision, prop, and traffic counts substantially, so streaming density and long-route performance will need continued profiling as whole-map coverage grows.

## Next cycle goal

Cycle 101 should deepen the Los Angeles map foundation:

- add secondary connector or frontage-road logic around the widened primary corridor skeleton so the map starts breaking out of a pure rectangular arterial grid
- spread hotspot, staging, and district-pressure hooks across the new outer tiles so the larger LA footprint is not just renderable but starts becoming systemically playable
- keep the new whole-map metadata generated from shared layout and corridor tables instead of backsliding into bespoke per-block link authoring
