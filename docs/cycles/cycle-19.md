# Cycle 19 Report

Date completed: April 17, 2026

## Goal

Deepen the combat encounter by adding:

- a more deliberate hostile repositioning pass so the lookout can break static lanes and force the player to move between pieces of cover
- a clearer player-side recovery and fail-state presentation layer that starts to bridge from debug text toward an actual HUD
- another authored sandbox pass that adds one or two stronger firing angles, making the current cover pieces matter more spatially instead of only logically
- another integration check around vehicle escape, re-entry to the lane, and encounter reacquisition so pressure systems stay compatible with the traversal slice

## What shipped

- `Sources/EngineCore/include/engine_core.h` and `Sources/EngineCore/engine_core.c` now give the lookout a pressure-anchor model instead of a single pacing strip. The hostile tracks a current firing angle, can retarget between west, center, east, and backline anchors, waits through a reacquire window after repositioning, and explicitly cools off when the player escapes into a vehicle before re-entering the lane.
- `Sources/EngineCore/engine_core.c` now stages the north combat pocket with a couple of extra authored blockers that make those firing angles matter: a new east-side low barrier, a west-side post, and another planter pass that create clearer sightline breaks and make the lookout’s movement legible through the environment rather than only through numbers.
- `Sources/MusicToDriveBy/Renderer.swift` now surfaces the new encounter rhythm in both the debug state and the world itself. The combat summary calls out the lookout’s active angle and reacquire timing, while the renderer adds anchor markers and route hints so it is obvious when the hostile is shifting to a new lane instead of simply jittering in place.
- `Sources/MusicToDriveBy/GameViewModel.swift` and `Sources/MusicToDriveBy/GameRootView.swift` now provide a lightweight in-game combat HUD layered directly over the Metal scene. The player gets a top encounter banner, a condition card, weapon and encounter cards, and a prompt pill that summarize health, weapon state, current pressure, reset windows, and vehicle escape or re-entry status without needing to rely exclusively on the floating debug panel.

## Notes on implementation

- The lookout still uses authored anchors rather than full navigation. That keeps the movement readable and controllable for the current sandbox, but it is still a prototype pressure system rather than a general enemy locomotion framework.
- The new HUD is intentionally narrow. It does not replace the debug window or try to become the final game UI yet; it simply promotes the most important combat information into the actual play view so iteration is faster.
- Vehicle escape and re-entry are handled as combat reacquire timing, not as a full pursuit system. The lookout settles back, drops immediate pressure, and rebuilds the encounter after the player steps out again, but it does not chase vehicles or communicate with any wider systemic response yet.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk into the north combat lane, pick up the lead pipe and pistol with `T`, and confirm the new HUD appears on top of the scene with health, weapon, encounter, and prompt panels.
- Stay exposed long enough for the lookout to pressure you, then move across the lane and confirm the encounter panel and world markers report a new firing angle instead of leaving the hostile in one static strip.
- Use the west post, center cover, and east barrier in sequence and confirm the lookout shifts angles to keep pressure on the player while pistol shots still stop on the first blocking prop.
- Enter a staged vehicle after the lookout is active, drive a short distance, then exit back near the lane and confirm the HUD reports vehicle escape or reacquire timing while the lookout rebuilds pressure instead of instantly firing again.
- Fire until the clip runs low, press `Y` to reload, and confirm the HUD, prompt text, hostile angle shifts, and staged vehicle handoff prompts remain readable together.

## Blockers and risks

- The hostile still repositions through a handcrafted lane model rather than general-purpose AI movement, so this work improves pressure and readability without yet proving broader enemy navigation.
- The new HUD is a bridge layer, not the final interface. It improves play readability a lot, but there is still no minimap, reticle design pass, fail screen, or proper accessibility treatment around the combat readout.
- Vehicle escape now cools combat pressure correctly inside the lane, but there is still no true pursue, search, or witness logic beyond this local encounter. The wider systemic response phase remains ahead of the project.

## Next cycle goal

Cycle 20 should begin the first systemic-response pass by adding:

- a first witness or bystander reaction loop that notices gunfire or melee pressure inside the lane and shifts into flee or investigate behavior
- a lightweight hostile search state so breaking line of sight and re-entering from a vehicle starts to feel like a broader response loop instead of a purely local cooldown
- another HUD pass that clarifies combat versus systemic pressure when the lane is no longer only about one hostile and one player
- another integration check proving traffic, civilian reaction, hostile pressure, and staged vehicle escape can share the same slice without collapsing readability
