# Cycle 55 Report

Date completed: April 18, 2026

## Goal

Spend a cycle directly attacking the most obvious graybox silhouettes that still make the prototype feel cheap:

- rebuild the shared non-player human figure so ambient civilians, witnesses, bystanders, and territory edge agents stop reading as stacked boxes
- reshape the shared staged and ambient vehicle silhouettes so they read with a clearer hood, cabin, roof, and wheel structure
- upgrade the static parked-car helper so the street does not mix richer moving traffic with blocky parked slabs

## What shipped

- `Sources/MusicToDriveBy/Renderer.swift` now replaces the old five-box pedestrian placeholder with a layered human silhouette built from separate hips, torso, shoulders, neck, head, upper and lower legs, feet, and arm sections. That shared builder still drives ambient civilians plus the authored witness, bystander, and territory edge agents, so the whole street population now lifts together.
- `Sources/MusicToDriveBy/Renderer.swift` now adds reusable local vehicle-part, light-pair, and rounded-wheel helpers, then uses them to rebuild bicycles, mopeds, motorcycles, coupes, and sedans with more believable body taper and layered tires instead of one or two slab-like boxes.
- `Sources/EngineCore/engine_core.c` now upgrades `push_parked_car` from a two-box silhouette to a fuller parked street car with lower body, hood, trunk, cabin, window band, bumper trim, and layered wheels, so static curbside vehicles no longer lag behind the moving traffic pass.
- `README.md` now advances the repo to Cycle 56, adds manual checks for the new human and vehicle silhouette pass, and updates the prototype summary to call out the stronger civilian and traffic forms.

## Notes on implementation

- This pass stays within the current box-primitive rendering language. The improvement comes from better composition and proportion, not from introducing imported meshes or a new asset pipeline.
- The rounded-wheel look is still an approximation built from layered tire slices and hubs, but it breaks the “cube wheel” read that was dragging down both moving and parked cars.
- The upgraded pedestrian builder intentionally keeps the same overall height band and role-accent attachment area so the existing alert rings, chest markers, and role readouts remain aligned.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk around ambient civilian spawn points and confirm the shared civilian silhouettes now read with separate shoulders, arms, legs, and heads instead of single-torso mannequins.
- Revisit the witness, bystander, and territory edge agents and confirm their alert markers still line up while the bodies now look closer to people than to stacked props.
- Compare a staged sedan or coupe, an ambient moving vehicle, and a parked curbside car and confirm they now share the same broader hood, cabin, and roof layering instead of mixing richer moving traffic with blocky static cars.
- Stop beside a bicycle, moped, motorcycle, coupe, or sedan and confirm the wheels now read as rounded layered tires with visible hubs rather than as single cuboids.

## Blockers and risks

- These silhouettes are still built entirely from procedural boxes, so they improve proportion and readability without yet reaching mesh-driven character or vehicle fidelity.
- Parked cars are richer than before, but building facades, props, and many roadside objects still carry obvious graybox massing that will need similar attention.
- The shared builders now put more geometry on screen per actor and vehicle, so a later large-world pass should keep an eye on density and LOD strategy as the city scale grows.

## Next cycle goal

Cycle 56 should keep pushing the world away from graybox presentation:

- target another high-visibility realism pass on static streetscape massing such as curb furniture, rooflines, transit shelters, or corner props
- keep aligning parked, staged, and ambient city elements so one part of the street does not look materially more placeholder than another
- continue raising the perceived production bar without breaking the current traversal, combat, and territory readability
