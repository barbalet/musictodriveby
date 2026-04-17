# Cycle 16 Report

Date completed: April 17, 2026

## Goal

Extend combat foundations by adding:

- a first firearm scaffold with pickup, equip, and readable ammo or reload state
- a narrow ranged-hit implementation that can share the same dummy or another simple target without breaking the melee loop
- stronger combat-state presentation so melee, firearms, and vehicle prompts remain readable together
- a follow-up integration pass that keeps combat inputs and ownership transitions stable while moving between on-foot and vehicle play

## What shipped

- `Sources/EngineCore/include/engine_core.h`, `Sources/EngineCore/engine_core.c`, and `Sources/MusicToDriveBy/InputController.swift` now support a first firearm layer on top of the existing melee scaffold. The prototype has a pistol pickup in the world, slot-based weapon selection with `1` and `2`, a dedicated `Y` reload action, clip and reserve ammunition tracking, and a lightweight ranged-hit path that can damage the same practice dummy without interfering with the lead-pipe loop.
- `Sources/MusicToDriveBy/Renderer.swift` now renders firearm-facing combat cues directly in the graybox slice. The pistol pickup has its own marker language, the third-person actor can visibly hold the pistol, recent shots produce muzzle and impact flashes, and the practice dummy remains the shared target for both close-range and ranged validation so the sandbox reads as one coherent combat test instead of two disconnected prototypes.
- `Sources/MusicToDriveBy/GameViewModel.swift` and `Sources/MusicToDriveBy/Renderer.swift` now expose more useful combat state in the overlay. The combat line reports the active slot, clip and reserve ammo, reload timing, and dummy state, while the prompt line now balances pickup, fire, reload, weapon-switch, and vehicle-enter/exit guidance instead of forcing melee or vehicle messages to crowd each other out.
- Cycle 16 also completed the intended integration pass with the traversal slice. Entering a vehicle keeps weapon ownership and the selected slot intact, pistol reloads cancel cleanly instead of leaking across traversal modes, and exiting back to on-foot play restores the chosen weapon state so the player can keep testing melee or firearms without a separate recovery path.

## Notes on implementation

- The firearm scaffold is deliberately narrow: one pistol, one ammo model, one reload action, and one shared dummy target. This is enough to prove that ranged combat can live in the same engine/input/render path as melee without pretending the broader firearm roster is solved.
- Ranged hits are still prototype-friendly rather than fully simulated ballistics. The pistol uses a simple forward shot test against the practice dummy, plus readable muzzle and impact flashes, so the game now has usable firearm feedback without yet needing full projectile objects, decals, or cover interactions.
- Slot switching and reload behavior are intentionally lightweight. The current implementation prioritizes readable ownership transitions and stable prompts over a full inventory UI, magazine varieties, or animation-driven reload timing.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk north along the starting sidewalk, pick up the lead pipe and the pistol with `T`, and confirm `1` equips the pipe while `2` equips the pistol once both are owned.
- With the pistol equipped, fire at the practice dummy using `Space` or captured left click and confirm the combat line updates clip and reserve ammo while the world shows muzzle and impact flashes.
- Spend enough rounds to leave the clip partially empty, press `Y`, and confirm the pistol enters a reload state, the prompt reports reload timing, and the clip refills from reserve ammo when the timer completes.
- Enter a staged vehicle with the pistol selected and confirm the combat line reports the weapon as stowed while driving; then exit back to on-foot play and confirm the selected slot and remaining ammo persist correctly.
- Switch back to the lead pipe after using the pistol and confirm melee attacks still connect against the same dummy without breaking the reset, health, or prompt flow.

## Blockers and risks

- Firearms still cover only one pistol and one dummy target. There is no projectile persistence, spread model, enemy return fire, or broader weapon taxonomy yet.
- Reloading and slot switching are readable, but they are still systemic scaffolds rather than animation-driven interactions. There is no holster model, no partial reload behavior, and no visual inventory representation beyond the actor pose and overlay text.
- The prompt system is now better balanced, but a larger combat sandbox will eventually need a more deliberate HUD so weapon state, damage feedback, and vehicle interaction cues do not compete for the same debug-style text channel.

## Next cycle goal

Cycle 17 should deepen combat foundations by adding:

- a second combat target or simple hostile placeholder so ranged and melee tests are no longer limited to one stationary dummy
- stronger hit feedback and target-state clarity, especially around impact response, defeat/reset readability, and aim validation
- a small combat-sandbox layout pass that stages pickups, target spacing, and lines of sight more intentionally inside the current neighborhood slice
- another integration check to keep combat-state prompts, weapon ownership, and traversal transitions stable while the sandbox gets busier
