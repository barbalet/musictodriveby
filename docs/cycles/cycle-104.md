# Cycle 104 Report

Date completed: April 18, 2026

## Goal

Start tying the widened branch network into systemic play:

- reuse the branch-route metadata to bias hotspot, civilian, or traffic selection toward specific outer branch nodes so those routes affect encounter texture and not just traversal staging
- add another shared layer of branch-side world structure, landmarking, or frontage variation so the widened outer routes keep moving closer to believable local-street fabric
- keep the new systemic branch pass procedural and compatible with the longer-term full-map road-graph expansion

## What shipped

- `Sources/EngineCore/engine_core.c` now adds a shared outer-route node context helper so the widened branch-end marker, support, hotspot, spawn, and passive-traffic systems can all reuse the same branch-node geometry instead of each recomputing its own offsets.
- The outer branch nodes now carry extra support-node hotspots and extra pedestrian pull points clustered around the marked branch ends, so widened branch routes affect encounter and civilian texture more than a single deeper landmark hook.
- The shared branch-support pass now also adds frontage-specific dynamic route signaling at those outer branch ends, giving residential, transit-market, civic-retail, and service-spur branch nodes a clearer systemic identity beyond static furniture alone.
- Passive stop-zone pressure now extends onto the marked support nodes as well as the earlier branch-center and node positions, so the widened outer routes read as lightly active destinations rather than only darker side-road geometry with one mid-branch pressure pulse.
- `README.md` now advances the repo to Cycle 105, adds manual checks for the new branch-node activity pass, and updates the prototype summary with the latest widened-route systemic layer.

## Notes on implementation

- The new node-context helper centralizes branch axis, support shift, marker position, support position, pad position, and node position so the widened route systems stay aligned as branch logic grows.
- The support-node encounter pass reuses that same context inside both the hotspot builder and the traffic-occupancy refresh, which keeps the new civilian and pressure beats spatially anchored to the visible branch-end marker kit.
- Dynamic route signaling stays lightweight and procedural: each frontage template swaps the prop kind and color language, but the placement flow still comes entirely from the shared branch-node context.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Visit several widened outer branch ends and confirm hotspot or civilian activity now clusters around the marked support nodes instead of living only at the branch midpoint or deeper landmark hook.
- Compare residential, transit-market, civic-retail, and service-spur branch nodes and confirm the dynamic route-signaling layer differs by frontage feel instead of repeating one branch-end accent everywhere.
- Move through the widened outer tiles and confirm the street or system summary now carries a second passive stop-zone beat around the marked support nodes so the branch ends feel like light route destinations.

## Blockers and risks

- The widened branch nodes now feel more systemically alive, but they are still part of a coarse procedural branch network rather than true neighborhood-level road and parcel reconstruction from the linked LA gang map.
- Civilian and hotspot selection now leans harder toward branch ends, but full encounter logic, mission beats, and traffic path choice still mostly center on the larger arterial skeleton.
- The new dynamic route-signaling layer improves local identity, but the widened map still needs richer branch-side massing, frontage depth, and route-aware simulation to fully stop reading as an enlarged prototype.

## Next cycle goal

Cycle 105 should keep pushing the widened branch routes toward believable local-street fabric:

- add another shared layer of branch-side physical structure, frontage breakup, or local landmarking so widened branch corridors stop falling back to simple road-plus-support-node patterns
- reuse the branch-node context to bias more encounter-facing systems, such as deeper hotspot selection or route-adjacent staging, so branch routes matter more to play pacing
- keep the widened-route pass procedural and aligned with the longer-term full-map Los Angeles road-graph buildout
