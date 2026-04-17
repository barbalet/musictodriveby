# Cycle 17 Report

Date completed: April 17, 2026

## Goal

Deepen combat foundations by adding:

- a second combat target or simple hostile placeholder so ranged and melee tests are no longer limited to one stationary dummy
- stronger hit feedback and target-state clarity, especially around impact response, defeat/reset readability, and aim validation
- a small combat-sandbox layout pass that stages pickups, target spacing, and lines of sight more intentionally inside the current neighborhood slice
- another integration check to keep combat-state prompts, weapon ownership, and traversal transitions stable while the sandbox gets busier

## What shipped

- `Sources/EngineCore/include/engine_core.h` and `Sources/EngineCore/engine_core.c` now support a two-target combat lane instead of a single dummy test. The original practice dummy remains, and a new lookout-style hostile placeholder now patrols beside it with health, reaction, alert, reset, and focus state tracked directly in the engine so both melee and pistol hits can resolve against whichever target the player is actually lining up.
- `Sources/EngineCore/engine_core.c` now stages the north-side sandbox more deliberately. The pipe and pistol pickups were repositioned into a tighter lane, the targets now sit on separated backstops, and the scene adds low cover, planters, bollards, and a simple practice slab so the slice reads like a combat test pocket instead of a loose set of props dropped onto open sidewalk.
- `Sources/MusicToDriveBy/Renderer.swift` now makes target-state readability much stronger. Both targets render at once with distinct silhouettes, health bars, range or alert rings, focus markers, and stronger hit flashes, while the combat text now reports dummy state, lookout state, and current focus so aim validation is visible even before a shot or melee swing lands.
- Cycle 17 also kept the existing traversal and vehicle integration intact while the sandbox got busier. Weapon ownership and slot selection still persist across entering and exiting vehicles, combat prompts still hand off cleanly when a weapon is stowed for driving, and the new focus or hostile state does not break the existing on-foot versus vehicle control split.

## Notes on implementation

- The new lookout is still a hostile placeholder rather than a full enemy. It patrols, alerts toward the player, takes damage, and resets, but it does not yet attack back, pathfind, use cover, or coordinate with any larger encounter system.
- Aim validation is intentionally lightweight and readable. The engine now tracks a focused combat target, approximate aim alignment, and the last target hit so the renderer can explain why melee and pistol attacks are or are not connecting without yet needing a full HUD, reticle system, or ballistic simulation.
- The new sandbox props improve spacing and movement readability, but ranged hits still use prototype target tests rather than true line-of-sight occlusion. The lane now looks more intentional, yet the cover is not fully authoritative for combat logic until a later pass.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk into the north combat lane, pick up the lead pipe and pistol with `T`, and confirm both pickups now sit inside the staged practice area instead of the earlier loose sidewalk placement.
- Toggle third person with `C` or `Tab`, watch both targets, and confirm the dummy and lookout each show distinct rings, health bars, and reset or alert feedback as you move around the lane.
- Equip the lead pipe with `1`, close on either target, and confirm the combat line reports the focused target while melee hits and hit flashes can land on either the dummy or the lookout instead of always resolving against one fixed target.
- Equip the pistol with `2`, fire at each target with `Space` or captured left click, and confirm the focus summary, muzzle flash, impact flash, and target-health feedback all update correctly for the target you are aiming at.
- Enter a staged vehicle after picking up both weapons, confirm the combat prompt reports the active slot as stowed while driving, then exit back to on-foot play and confirm the sandbox still reports the correct weapon slot, target focus, and target state.

## Blockers and risks

- The lookout is still non-lethal. There is no enemy attack loop, no player health, no suppression or flee behavior, and no encounter escalation beyond alert and damage reaction.
- Cover is only partially systemic. The new props affect movement and make the lane easier to read, but they do not yet block firearm hit tests, so the sandbox can visually suggest protection before the combat logic truly respects it.
- Combat feedback is much clearer now, but it still lives in a debug-style overlay. As the slice gains hostile offense and more targets, the project will need a more deliberate HUD layer so combat readability does not collapse under system growth.

## Next cycle goal

Cycle 18 should turn the placeholder hostile lane into the first actual combat encounter by adding:

- a first hostile attack loop plus player damage or recovery feedback so the sandbox is no longer one-sided
- cover-aware firearm validation so the staged props begin to matter to line-of-sight and shot outcomes
- a simple encounter reset or pacing pass that keeps the two-target lane readable once the hostile can pressure the player
- another integration check proving hostile pressure, weapon handling, and vehicle transitions can coexist without breaking the current traversal slice
