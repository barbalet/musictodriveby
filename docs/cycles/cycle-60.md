# Cycle 60 Report

Date completed: April 18, 2026

## Goal

Widen the shared realism layer from surfaces into curbside infrastructure so the expanded Los Angeles slice stops reading like a clean procedural curb:

- add reusable curbside props such as hydrants, parking meters, signal-control boxes, utility cabinets, and separate bus-stop signs
- thread those props through the shared shelter, intersection, frontage, and service-court builders instead of treating them as one-off landmarks
- make repeated corners and frontage edges feel more like serviced city blocks without breaking the procedural scaling path

## What shipped

- `Sources/EngineCore/engine_core.c` now adds shared helpers for fire hydrants, parking meters, signal-control boxes, utility cabinets, and bus-stop signs so the curbside pass can be reused across the widening map.
- `Sources/EngineCore/engine_core.c` now upgrades the shared bus shelter builder with a separate stop sign and nearby utility cabinet so transit edges read like actual stops instead of glass boxes dropped at the curb.
- `Sources/EngineCore/engine_core.c` now threads signal-control boxes and hydrants into the shared intersection builder so corners pick up basic city-service clutter alongside the richer signal poles and planters.
- `Sources/EngineCore/engine_core.c` now places parking meters, hydrants, and utility cabinets across hub, mixed-use, residential, service-spur, and service-court frontages so long curb runs stop reverting to empty clean sidewalks.
- `README.md` now advances the repo to Cycle 61, adds manual checks for the curbside infrastructure pass, and updates the prototype summary to call out the new serviced-street layer.

## Notes on implementation

- This cycle stays inside the existing shared box-based streetscape language, but it shifts the realism work toward how transit edges, storefront curbs, and signalized corners are furnished and maintained.
- The prop placements intentionally live inside the shared frontage and node builders so every reused block template inherits the curbside detail automatically rather than depending on bespoke authored dressing.
- The new helpers stay compact enough to preserve the procedural approach while still breaking up repeated curb reads with recognizable city-service silhouettes.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Stop beside several bus shelters and confirm each one now carries a separate route sign and utility cabinet rather than ending at the shelter frame.
- Walk hub, mixed-use, residential, and service-spur frontages and confirm the curb now picks up parking meters, hydrants, and cabinet detail in plausible positions instead of staying visually empty.
- Pause at multiple intersections and confirm the corners now include signal-control boxes and hydrants in addition to the richer signal poles and planter kit.
- Compare different block templates and confirm the curbside infrastructure still feels shared and scalable rather than like one bespoke hero corner.

## Blockers and risks

- The new curbside layer improves plausibility, but it still relies on stylized box-composed props rather than authored meshes or textures, so the final production pass will need more geometric nuance.
- As the many-mile map grows, the repeated curbside kit will need density tuning so utility clutter helps realism without overloading sightlines or scene-box count.
- Road and frontage realism are improving faster than district-scale geographic specificity, so later cycles still need to keep pushing toward the full Los Angeles gang-map footprint and less blocky macro layout.

## Next cycle goal

Cycle 61 should keep pulling the streets away from graybox repetition:

- target lane-edge and streetside realism that makes the road corridor itself feel more professionally built, not just the sidewalks
- continue refining the shared procedural kit so repeated miles of frontage and roadway can read less blocky without switching to bespoke hand-authored blocks
- preserve the scalable helper-based approach needed for the full-map Los Angeles plan
