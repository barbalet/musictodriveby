# Cycle 23 Report

Date completed: April 17, 2026

## Goal

Keep tightening the pre-vertical-slice systemic loop by adding:

- another pass on civilian spread so the reaction can extend one step farther into adjacent sidewalk or street activity without requiring a full population system
- another pass on incident cooldown and reset so combat escape, civilian recovery, and traffic normalization feel cleaner after the lane settles
- another readability pass that keeps multi-civilian response, traffic easing, and traversal prompts legible when all three overlap
- another integration check across on-foot combat, staged vehicle escape, civilian spillover, and ambient traffic so the Cycle 24 milestone can focus on consolidating the first believable vertical slice

## What shipped

- `Sources/EngineCore/engine_core.c` now lets the street incident spill one step farther into the surrounding road network. The main combat block still carries the strongest response, but the nearest adjacent link can now pick up weaker incident occupancy so the traffic reaction reads more like a neighborhood disturbance than a single-lane toggle.
- `Sources/EngineCore/engine_core.c` also adds a more explicit street-normalization phase. Once hostile search, recent gunfire, and civilian panic have fallen away, incident timers, traffic pressure, and player recovery settle through a cleaner cooldown window instead of lingering noisily or dropping straight from hot to calm.
- `Sources/MusicToDriveBy/Renderer.swift` now visualizes those hotter and cooler states more clearly. Nearby blocks and road links warm up when the incident spills outward, then shift toward cooler tones when the lane is normalizing so the player can read the difference between active pressure and recovery at a glance.
- `Sources/MusicToDriveBy/Renderer.swift` also improves the street/system readout so normalization is called out directly, helping the HUD distinguish between live escalation, cooling pressure, and the final return toward normal ambient movement.

## Notes on implementation

- Adjacent spillover still uses lightweight traffic occupancies rather than broader AI coordination. That keeps the change cheap and readable while still widening the systemic footprint of the incident.
- Street normalization is intentionally conservative. It only accelerates once hostile search, fresh shots, and agitated civilian states have all dropped out, which helps avoid false "all clear" messaging in the middle of a fight.
- The new warm/cool rendering cues are meant to support the HUD, not replace it. The prototype still depends on the combined overlay plus in-world feedback while the vertical slice is being stabilized.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk north into the combat lane, grab the lead pipe or pistol with `T`, and confirm the HUD still shows separate health, weapon, encounter, street, and prompt panels.
- Trigger the first witness with gunfire or a close melee hit, then keep pressure on the lane and confirm the second authored bystander still joins the street response instead of leaving the block to one actor.
- Break line of sight behind cover and confirm the lookout still searches and settles through a reacquire window while the civilian response continues to read clearly around it.
- Cross near the live incident and confirm traffic now eases back not just on the combat link but also on the nearest adjacent street link while nearby block and link feedback warms with the spillover.
- Back off after the lookout search and civilian panic have resolved, and confirm the street eventually enters a clear normalization phase with cooler ambient and road-link feedback instead of snapping straight from danger to calm.
- Wait through the cooldown and confirm ambient pedestrians and traffic return more cleanly once the block settles.

## Blockers and risks

- The incident now reaches farther, but it is still fundamentally a local slice mechanic rather than a district-scale systemic response model.
- Normalization is more readable than before, but the exact timing will still need hands-on tuning in the live app so it does not feel either too abrupt or too sluggish.
- The HUD is holding together, but this phase is close to the point where every added systemic state risks turning readability gains back into overlay clutter.

## Next cycle goal

Cycle 24 should consolidate the end-of-phase vertical slice by focusing on:

- a milestone integration pass that tightens traversal, combat, civilian reaction, and traffic response into one stable playable loop
- another clarity pass on the transitions between active incident, cooling pressure, street normalization, and fully calm ambient state
- another integration check around vehicle escape, hostile reset, civilian recovery, and returning traffic so the full slice feels intentionally authored rather than loosely accumulated
- a milestone readout pass that frames Cycles 20-24 as the first believable combined traversal/combat/systemic-response vertical slice
