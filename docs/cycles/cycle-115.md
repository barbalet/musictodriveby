# Cycle 115 Report

Date completed: April 19, 2026

## Goal

Keep deepening widened branch fabric into fuller built street edges:

- add another shared layer of rear-core massing, court interior closure, or service-yard backstop structure so widened branches carry denser local-street coverage beyond the new core-backstop layer
- bias more localized pedestrian pull, parked density, or branch-lane hesitation toward that deeper shared branch geometry so widened branches keep gaining traversal texture
- keep the widened-branch pass procedural and compatible with the long-term full-map Los Angeles road and parcel buildout

## What shipped

- `Sources/EngineCore/engine_core.c` now adds a deeper shared rear-core layer behind the widened-branch core-backstop structures, extending those branch pockets into frontage-specific garden-wall massing, market readout cores, service-yard backstop walls, and tighter civic rear-court closures instead of letting the branch fabric stop at the final backstop wall.
- Residential, transit-market, service-spur, and civic-retail widened branches now each get different rear-core language, including calmer residential garden-wall or court-nook massing, taller market readout cores, heavier service-yard backstop walls, and tighter civic rear-court closures.
- The widened branch hotspot pass now adds a deeper rear-core hotspot plus extra pedestrian pull around that same rear-core geometry, so the deepest massing layer carries localized foot traffic instead of remaining a visual-only depth cue.
- The widened branch vehicle handoff pass now stages another parked anchor deeper into that same rear-core pocket, adding one more layer of parked density that stays aligned with the new deepest massing geometry.
- The passive outer-route pressure pass now adds another stop-zone hesitation layer around the rear-core positions and pulls, so widened branches influence low-intensity traffic choice farther into the branch interior instead of tapering out at the core-backstop layer.
- `README.md` now advances the repo to Cycle 116, adds manual checks for the new rear-core pass, and updates the prototype summary with the latest widened-branch realism layer.

## Notes on implementation

- The new rear-core layer stays procedural by deriving all placement from the shared outer-route node context plus the earlier core-backstop helpers instead of introducing bespoke authored rear-lot geometry.
- The added pedestrian pull, parked staging, and passive hesitation layer all reuse the same rear-core positions, which keeps the new geometry and the systems it influences spatially aligned.
- Frontage-specific dimensions and prop language keep the pass compatible with the long-term full-map parcel buildout while helping widened branches read like different local-street conditions instead of one repeated deepest-massing kit.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Visit several widened outer branch ends and confirm each one now carries a deeper rear-core massing, court interior closure, or service-yard backstop structure behind the core-backstop layer instead of letting the branch fabric stop at the final backstop wall.
- Compare residential, transit-market, civic-retail, and service-spur branch depths and confirm the new rear-core layer changes by frontage feel, with calmer residential garden-wall or court-nook massing, taller market readout cores, heavier service-yard backstop walls, and tighter civic rear-court closures instead of one repeated deepest branch cap.
- Move on foot and by vehicle through widened outer branches and confirm the deeper rear-core hotspot, extra pedestrian pull, added parked staging, and another hesitation beat now sit on that same rear-core geometry instead of making the deepest massing layer visually present but systemically quiet.

## Blockers and risks

- The widened branches now read with a deeper rear-core massing layer, but the new garden-wall massing, court closures, and service-yard backstop walls are still procedural support geometry rather than true parcel-by-parcel Los Angeles reconstruction from the linked gang map.
- Localized pedestrian pull, parked density, and hesitation now extend farther into the widened branches, but broader civilian routing, active traffic pathfinding, and mission logic still lean toward the main arterial skeleton.
- The added rear-core layer improves local-street silhouette and traversal texture, but the outer map still needs denser building coverage, more continuous block-face reconstruction, and more realistic district-by-district parcel logic before it will read like a professional full-city space.

## Next cycle goal

Cycle 116 should keep deepening widened branch fabric into fuller built street edges:

- add another shared layer of court-end detail, rear-yard enclosure, or service-yard terminal structure so widened branches carry denser local-street coverage beyond the new rear-core layer
- bias more localized pedestrian pull, parked density, or branch-lane hesitation toward that deeper shared branch geometry so widened branches keep gaining traversal texture
- keep the widened-branch pass procedural and compatible with the long-term full-map Los Angeles road and parcel buildout
