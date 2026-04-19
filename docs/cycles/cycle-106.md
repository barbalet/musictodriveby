# Cycle 106 Report

Date completed: April 18, 2026

## Goal

Keep deepening the widened branch corridors into believable local streets:

- add another shared layer of branch-side frontage massing, lot breakup, or small landmark structure so widened branches stop reading like arterial offshoots with only support-node furniture
- bias more route-adjacent staging or encounter-facing systems, such as vehicle placement, civilian pull, or hotspot ranking, toward those shared branch contexts so the outer branches matter more to play pacing
- keep the widened-branch pass procedural and compatible with the long-term full-map Los Angeles road and parcel buildout

## What shipped

- `Sources/EngineCore/engine_core.c` now adds a shared branch-frontage massing helper on top of the outer-route node context, so widened branch support nodes pick up frontage-specific aprons, curb seams, shallow walls, and canopy structure instead of stopping at darker branch asphalt plus loose furniture.
- Residential, transit-market, service-spur, and civic-retail outer branches now each get different small support-side massing language, which gives those widened local streets more physical identity without breaking the procedural branch layout.
- The widened branch vehicle handoff pass now reuses the shared outer-route node context for deeper branch anchor depth and support-side pedestrian pull, so branch-side staging sits nearer to the visible support-node frontage kit instead of hanging off a separate generic branch offset.
- The earlier support-node hotspot and passive-pressure layers stay aligned with that same geometry, so the widened branch ends now read more like small local stopping pockets with structure and staging rather than lightweight route accents on top of a flat branch strip.
- `README.md` now advances the repo to Cycle 107, adds manual checks for the new branch-frontage and support-aligned staging pass, and updates the prototype summary with the latest widened-route realism layer.

## Notes on implementation

- The new frontage-massing helper stays procedural by deriving all of its orientation from the shared outer-route node context, then only swapping dimensions, colors, and roof or wall proportions by frontage template.
- The branch-side massing pass adds a ground apron, a curb seam, a shallow support-side mass, and a roofed edge so widened branches carry more readable lot structure without requiring bespoke authored parcels.
- Vehicle handoff alignment now keys off the shared support distance and pad position when route context is available, which ties the deeper branch anchor and sidewalk pull point back to the same visible support-node composition.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Visit several widened outer branch ends and confirm each one now carries a shallow frontage apron plus a small wall or canopy mass on the support side instead of only darker branch asphalt and support furniture.
- Compare residential, transit-market, civic-retail, and service-spur branch edges and confirm the new branch-side massing changes by frontage feel instead of repeating one branch-side slab.
- Approach widened branch ends on foot and by vehicle and confirm the deeper branch vehicle handoff now stages off the same support-node frontage geometry and nearby sidewalk pull point instead of a separate generic branch offset.

## Blockers and risks

- The widened branches now have better physical edge definition, but the massing is still a coarse procedural frontage kit rather than a true parcel-by-parcel Los Angeles reconstruction from the linked gang map.
- Branch-side staging now sits more coherently against the support nodes, but traffic path choice, civilian routing, and mission logic still largely prioritize the main arterial skeleton over the outer branch fabric.
- The new canopies, aprons, and shallow walls improve readability, but the outer map still needs deeper lot composition, more continuous street-edge variety, and higher-fidelity world structure to fully reach the professional look the project is aiming for.

## Next cycle goal

Cycle 107 should keep pushing the widened branch fabric away from enlarged graybox street logic:

- add another shared layer of branch-side lot composition, parcel-edge breakup, or small landmark structure so widened branches carry more believable street-edge depth beyond the new apron and canopy massing
- bias more route-adjacent simulation, such as parked-vehicle pockets, civilian clustering, or low-intensity branch traffic choice, toward those shared branch contexts so the outer branches matter more to traversal and pacing
- keep the widened-branch pass procedural and compatible with the long-term full-map Los Angeles road and parcel buildout
