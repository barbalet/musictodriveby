# Cycle 22 Report

Date completed: April 17, 2026

## Goal

Keep building toward the Cycle 24 vertical-slice milestone by adding:

- another civilian-feedback pass so the street feels populated by more than one authored reaction source, even if that still uses lightweight placeholder logic
- another pass on incident spread and recovery so witness response, hostile search, and traffic easing can scale without becoming noisy or unreadable
- another HUD and world-feedback refinement that keeps combat pressure, street incident state, and traversal prompts legible when they overlap
- another integration check across lane combat, staged vehicle escape, traffic reaction, and broader ambient population activity so the systemic slice keeps converging on a believable first vertical slice

## What shipped

- `Sources/EngineCore/include/engine_core.h` and `Sources/EngineCore/engine_core.c` now add a second authored bystander on the far side of the block. That civilian uses the same lightweight state vocabulary as the first witness, but reacts primarily to the broader street incident instead of direct lane noise, giving the prototype another clear reaction source without pretending to be a full crowd simulation.
- `Sources/EngineCore/engine_core.c` now lets that extra civilian strengthen incident spillover into traffic occupancies. Witness and bystander movement both contribute local incident pressure, so the street response scales more naturally once multiple civilians are involved instead of keeping all traffic reaction pinned to one marker and one actor.
- `Sources/MusicToDriveBy/Renderer.swift` now renders the second authored civilian in the world and upgrades the ambient pedestrian layer so some nearby placeholder pedestrians peel away from the incident with warmer reaction cues instead of continuing along their default routes as if nothing happened.
- `Sources/MusicToDriveBy/Renderer.swift` also improves readouts around the mixed systemic state. The activity summary now reports reacting pedestrians and easing vehicles, while the street card can call out crowd movement or multiple civilian sources when the block is no longer just one witness plus one hostile search state.

## Notes on implementation

- The extra civilian is still authored and local. This is a deliberate middle step between one hard-coded witness and a true ambient population response model.
- Ambient pedestrians are reacting through route/tint bias rather than full AI state. That keeps the prototype cheap while still making the surrounding sidewalks feel meaningfully affected by the lane.
- Traffic, civilian reaction, and hostile search are still all mediated through lightweight occupancy and timer systems. The benefit is readability and integration; the tradeoff is that none of these are yet general-purpose simulation frameworks.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk north into the combat lane, grab the lead pipe or pistol with `T`, and confirm the HUD still shows separate health, weapon, encounter, street, and prompt panels.
- Trigger the first witness with gunfire or a close melee hit, then keep pressure on the lane and confirm a second authored bystander on the far side of the block also joins the response instead of leaving the street to one actor.
- Break line of sight behind cover and confirm the lookout still searches and settles through a reacquire window while the civilian response continues to read clearly around it.
- Watch the nearby sidewalks while the incident is active and confirm some ambient pedestrians now peel away from the block with visible reaction tinting instead of calmly following their original routes.
- Check the debug activity line and confirm it now reports reacting pedestrians and easing vehicles while the incident is live.

## Blockers and risks

- The street now feels more populated, but the civilian response is still built from authored actors plus lightweight ambient biasing rather than true shared pedestrian AI.
- Incident spread is more scalable than before, but it is still local to the current combat block and does not yet hand off into broader district-level consequence.
- The HUD and world markers remain readable in this slice, but continued growth in systemic feedback will need careful UI restraint to avoid turning the overlay into a debug dashboard again.

## Next cycle goal

Cycle 23 should keep tightening the pre-vertical-slice systemic loop by adding:

- another pass on civilian spread so the reaction can extend one step farther into adjacent sidewalk or street activity without requiring a full population system
- another pass on incident cooldown and reset so combat escape, civilian recovery, and traffic normalization feel cleaner after the lane settles
- another readability pass that keeps multi-civilian response, traffic easing, and traversal prompts legible when all three overlap
- another integration check across on-foot combat, staged vehicle escape, civilian spillover, and ambient traffic so the Cycle 24 milestone can focus on consolidating the first believable vertical slice
