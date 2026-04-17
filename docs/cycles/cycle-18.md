# Cycle 18 Report

Date completed: April 17, 2026

## Goal

Turn the placeholder hostile lane into the first actual combat encounter by adding:

- a first hostile attack loop plus player damage or recovery feedback so the sandbox is no longer one-sided
- cover-aware firearm validation so the staged props begin to matter to line-of-sight and shot outcomes
- a simple encounter reset or pacing pass that keeps the two-target lane readable once the hostile can pressure the player
- another integration check proving hostile pressure, weapon handling, and vehicle transitions can coexist without breaking the current traversal slice

## What shipped

- `Sources/EngineCore/include/engine_core.h` and `Sources/EngineCore/engine_core.c` now track player combat state directly in the engine. The combat lane has player health, recovery delay, damage pulse, reset grace time, lookout attack cadence, lookout muzzle-flash state, and explicit cover checks built on the same solid collision boxes that already define the authored graybox props.
- `Sources/EngineCore/engine_core.c` now gives the lookout a real ranged attack loop. Once alert and in range, it winds up, fires into the lane, damages the player when line of sight is clear, and visibly dumps shots into cover when the player breaks the lane with the low barriers or the tall post. The same pass also adds a lightweight encounter reset that restores the lane when the player gets overwhelmed instead of leaving the sandbox in a confusing half-failed state.
- `Sources/EngineCore/engine_core.c` also makes the pistol respect cover. Target focus can remain readable through a blocked angle, but actual shots now stop on the first blocking collision box and report a cover hit instead of incorrectly punching through authored props.
- `Sources/MusicToDriveBy/Renderer.swift` now surfaces the new pressure loop in the world and the overlay. The combat line reports player health, cover state, hostile fire windup, cover-blocked pistol shots, hostile shot outcomes, and encounter reset timing, while the 3D feedback adds player health and pressure rings, player hit flashes, hostile muzzle charge, hostile shot impacts, and distinct colors for blocked versus successful shots.

## Notes on implementation

- Cover validation currently uses the existing collision-box scene model. That keeps the behavior honest to the visible lane props, but it is still a prototype LOS solution rather than a full projectile, penetration, or partial-exposure simulation.
- The first hostile attack loop is intentionally readable before it is smart. The lookout winds up, fires, and respects cover, but it still does not flank, rush, suppress, or coordinate with other enemies.
- The encounter reset is tuned to protect iteration rather than to create a final fail state. When the player is dropped, the lane recenters quickly so repeated combat tests stay fast and understandable during this prototype phase.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk into the north combat lane, pick up the lead pipe and pistol with `T`, and confirm the combat line now reports player health, cover state, dummy state, lookout state, and target focus together.
- Equip the pistol with `2`, step behind the low cover or the tall post, and confirm the prompt calls out when cover is blocking the dummy or lookout while the shot impact lands on the prop instead of the target.
- Stay exposed in the lane long enough for the lookout to wind up and fire, then confirm the hostile shot feedback, player hit pulse, and player-health line all update together.
- Break line of sight after taking damage and confirm health begins recovering after a short delay while the combat prompt and state line stay readable.
- Let the lookout win an exchange once, confirm the lane recenters with a short reset grace window, then re-enter a staged vehicle and exit again to verify the hostile-pressure loop does not break weapon ownership, slot selection, or traversal handoff text.

## Blockers and risks

- The hostile still fights as a single readable sandbox actor. There is no squad logic, flanking, melee pressure, or territory escalation yet, so the encounter is more useful for tuning than for proving broader enemy behavior.
- Player survivability is still debug-forward. Recovery is automatic, there is no authored HUD or fail screen yet, and there are no pickups, armor, or longer-term consequences layered on top of the health model.
- Cover reads much better now, but it is still all-or-nothing. There is no crouch, lean, blind fire, penetration, ricochet, or partial-body exposure logic yet, so the lane will eventually need a richer exposure model to avoid prototype simplifications showing through.

## Next cycle goal

Cycle 19 should deepen the combat encounter by adding:

- a more deliberate hostile repositioning pass so the lookout can break static lanes and force the player to move between pieces of cover
- a clearer player-side recovery and fail-state presentation layer that starts to bridge from debug text toward an actual HUD
- another authored sandbox pass that adds one or two stronger firing angles, making the current cover pieces matter more spatially instead of only logically
- another integration check around vehicle escape, re-entry to the lane, and encounter reacquisition so pressure systems stay compatible with the traversal slice
