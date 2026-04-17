# Cycle 15 Report

Date completed: April 17, 2026

## Goal

Begin combat foundations by adding:

- a first melee-weapon scaffold with pickup, equip, and readable close-range attack timing
- a simple damage target or reaction pass so hits produce visible consequences instead of only traversal-state changes
- the first combat-facing overlay or prompt updates needed to keep weapon state readable without losing the current vehicle and traversal debug value
- a narrow integration check proving the project can keep on-foot combat groundwork compatible with the existing vehicle traversal slice

## What shipped

- `Sources/EngineCore/include/engine_core.h`, `Sources/EngineCore/engine_core.c`, `Sources/MusicToDriveBy/InputController.swift`, and `Sources/MusicToDriveBy/MetalGameView.swift` now support a first melee loop. The slice has a world lead-pipe pickup on the starting sidewalk, a dedicated `T` pickup action, `Space` or captured left click attacks, a staged windup/strike/recovery swing state, and a practice dummy that takes damage, reacts visibly, and resets after being dropped.
- `Sources/MusicToDriveBy/Renderer.swift` now renders the new combat landmarks directly into the graybox slice. The lead pipe pickup gets a glowing ground marker, the practice dummy responds with hit flashes and downed-state presentation, and third-person on-foot play now shows the actor carrying and swinging the pipe so attack timing reads more clearly than a debug-only state change.
- `Sources/MusicToDriveBy/GameViewModel.swift` and `Sources/MusicToDriveBy/Renderer.swift` now surface combat state alongside the existing vehicle and traversal diagnostics. The overlay adds a dedicated combat line, interaction prompts call out when to grab the pipe or swing it, and vehicle prompts now make it clear that the weapon is stowed while driving rather than silently disappearing.
- Cycle 15 also completed the intended compatibility check with the current vehicle slice. Entering a staged vehicle clears the live melee swing state without dropping ownership, combat prompts hand off cleanly to vehicle prompts, and exiting the vehicle returns the player to on-foot traversal with the pipe ready again instead of forcing a separate recovery path.

## Notes on implementation

- The first melee scaffold is intentionally narrow: one lead pipe, one world pickup, and one practice dummy. This proves the engine/render/input path for on-foot combat without pretending inventory, enemy behaviors, or broad weapon classes are solved.
- The practice dummy uses a simple health, reaction, and reset model instead of a full ragdoll, animation graph, or NPC state machine. Hits are now readable and persistent enough for iteration, but they are still authored sandbox feedback rather than systemic combat AI.
- Mouse-look capture remains the same as before. The first click still captures the view, and later captured left clicks become melee swings so the prototype can support combat without giving up the existing desktop camera flow.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk north along the starting west sidewalk, find the glowing lead pipe pickup, and press `T` to confirm the combat line changes from pickup state to equipped state.
- Toggle third person with `C` or `Tab`, then swing with `Space` or captured left click and confirm the right arm and pipe motion now show windup, strike, and recovery timing.
- Step into melee range of the practice dummy and land hits until it drops, then confirm the dummy flashes on impact, the combat line reports hit state and health loss, and the target resets after a short delay.
- Pick up the pipe, enter the nearby staged sedan with `F`, and confirm the overlay reports the weapon as stowed while driving; then exit the vehicle and confirm the pipe is still owned and ready on foot.

## Blockers and risks

- Combat still covers only one melee weapon and one dummy target. There is no inventory slot model, weapon switching, enemy AI, or death-state persistence beyond the local reset timer.
- The dummy reaction is readable, but it is not a full character response system. There are still no animations, knockback physics, faction responses, or combat-aware navigation changes.
- The overlay remains useful for prototyping, but real combat readability will eventually need cleaner HUD treatment so weapon state, vehicle state, and systemic world feedback can coexist without feeling like a debug console.

## Next cycle goal

Cycle 16 should extend combat foundations by adding:

- a first firearm scaffold with pickup, equip, and readable ammo or reload state
- a narrow ranged-hit implementation that can share the same dummy or another simple target without breaking the melee loop
- stronger combat-state presentation so melee, firearms, and vehicle prompts remain readable together
- a follow-up integration pass that keeps combat inputs and ownership transitions stable while moving between on-foot and vehicle play
