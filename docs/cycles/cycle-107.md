# Cycle 107 Report

Date completed: April 18, 2026

## Goal

Keep pushing the widened branch fabric away from enlarged graybox street logic:

- add another shared layer of branch-side lot composition, parcel-edge breakup, or small landmark structure so widened branches carry more believable street-edge depth beyond the new apron and canopy massing
- bias more route-adjacent simulation, such as parked-vehicle pockets, civilian clustering, or low-intensity branch traffic choice, toward those shared branch contexts so the outer branches matter more to traversal and pacing
- keep the widened-branch pass procedural and compatible with the long-term full-map Los Angeles road and parcel buildout

## What shipped

- `Sources/EngineCore/engine_core.c` now adds a shared outer-route parcel-composition helper so widened branch support nodes pick up frontage-specific pocket lots, rear edge breakup, and small parcel structures behind the earlier support-side massing.
- Residential, transit-market, service-spur, and civic-retail branch pockets now each get different low-level parcel language, including calmer fence-and-tree breakup, market queue-pad detailing, heavier service screens, and tighter civic-retail plinth structure.
- The widened branch hotspot pass now adds a second parcel-side hotspot and a nearby civilian pull point off that same shared route context, so the new parcel pockets affect low-intensity local activity instead of staying as visual-only lot depth.
- The widened branch vehicle handoff pass now adds a support-side parked-vehicle pocket aligned to that same parcel geometry, so low-intensity branch staging sits with the visible lot pocket instead of hanging off an unrelated branch offset.
- `README.md` now advances the repo to Cycle 108, adds manual checks for the new parcel-pocket and support-side staging pass, and updates the prototype summary with the latest widened-route realism layer.

## Notes on implementation

- The new parcel-composition layer reuses the same outer-route node context as the earlier support, hotspot, and vehicle passes, which keeps the new lot pocket and the low-intensity simulation hooks spatially aligned with the branch-end frontage kit.
- The parcel helper stays procedural: it derives its orientation from branch axis and route sign, then swaps lot depth, edge treatment, and small supporting elements by frontage template instead of introducing one-off authored branch parcels.
- Civilian clustering and parked-vehicle staging now branch from that same context, which makes the widened outer branches feel more like small local stopping pockets with believable edge depth rather than a cleaner branch-road variation alone.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Visit several widened outer branch ends and confirm each one now carries a deeper parcel pocket or edge breakup behind the support-side massing instead of stopping at the earlier apron and canopy layer.
- Compare residential, transit-market, civic-retail, and service-spur branch pockets and confirm the new parcel-edge language changes by frontage feel instead of repeating one rear-lot strip.
- Approach widened branch ends on foot and by vehicle and confirm the new parked-vehicle pocket and nearby civilian cluster now stage off that same support-side parcel pocket instead of floating as separate simulation hooks.

## Blockers and risks

- The widened branch ends now have more believable parcel depth, but the lot breakup is still a coarse procedural pocket system rather than true parcel-by-parcel Los Angeles reconstruction from the linked gang map.
- Low-intensity local activity now sits closer to the parcel edges, but full traffic path choice, civilian routing breadth, and mission logic still skew heavily toward the arterial backbone.
- The new parcel pockets improve the local-street read, but the outer map still needs deeper lot coverage, more continuous block-face variety, and more realistic regional road detail before it will feel like a professional full-city game space.

## Next cycle goal

Cycle 108 should keep increasing the realism and systemic weight of the widened branch fabric:

- add another shared layer of branch-side parcel depth, edge continuity, or small landmark structure so widened branches carry more convincing local-street composition beyond the new pocket lots
- bias more route-adjacent low-intensity simulation, such as branch traffic choice, parked-car density, or civilian clustering, toward those shared branch contexts so the widened branches matter more to traversal texture
- keep the widened-branch pass procedural and compatible with the long-term full-map Los Angeles road and parcel buildout
