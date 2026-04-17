# Cycle 20 Report

Date completed: April 17, 2026

## Goal

Begin the first systemic-response pass by adding:

- a first witness or bystander reaction loop that notices gunfire or melee pressure inside the lane and shifts into flee or investigate behavior
- a lightweight hostile search state so breaking line of sight and re-entering from a vehicle starts to feel like a broader response loop instead of a purely local cooldown
- another HUD pass that clarifies combat versus systemic pressure when the lane is no longer only about one hostile and one player
- another integration check proving traffic, civilian reaction, hostile pressure, and staged vehicle escape can share the same slice without collapsing readability

## What shipped

- `Sources/EngineCore/include/engine_core.h` and `Sources/EngineCore/engine_core.c` now include the first civilian witness state machine for the combat slice. A bystander lives just outside the lane, notices melee or firearm noise, moves into investigate, escalates into flee under stronger pressure, and then cools down back toward the sidewalk instead of leaving systemic reaction entirely abstract.
- `Sources/EngineCore/engine_core.c` now gives the lookout a lightweight last-seen search loop. Breaking line of sight, splashing shots into cover, or escaping into a vehicle can pin a search position in the lane, temporarily bias hostile anchor choice toward that last known spot, and delay direct firing until the search window closes.
- `Sources/MusicToDriveBy/Renderer.swift` now renders those systemic states into the world and into the debug-facing summaries. The combat summary includes street pressure alongside the dummy and lookout state, the witness is visible with state-dependent color cues, and the hostile search point is marked directly in the scene so the player can read when pressure has shifted from active fire into a check of the last seen position.
- `Sources/MusicToDriveBy/GameViewModel.swift` and `Sources/MusicToDriveBy/GameRootView.swift` now split the HUD more clearly between direct combat pressure and broader street response. The overlay keeps the health, weapon, and encounter cards from the prior cycle, then adds a dedicated street card so witness escalation and hostile search can be read separately from immediate incoming fire.

## Notes on implementation

- The witness is still a handcrafted local systemic actor, not a general pedestrian population system. That is intentional for this phase: it proves the response loop and its readability before the project scales to broader ambient populations.
- The hostile search state is still tied to the authored combat pocket rather than a full pursuit or navigation system. It is a readable "last seen position" layer that bridges between simple cooldowns and later broader consequence systems.
- The HUD changes are still intentionally narrow. They do not attempt to solve final UI, but they now distinguish direct combat pressure from street-level spillover so testing mixed states is easier.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk north into the combat lane, grab the lead pipe or pistol with `T`, and confirm the HUD now shows a dedicated street card alongside the health, weapon, encounter, and prompt panels.
- Fire the pistol or land a melee hit near the witness and confirm the bystander shifts into investigate before escalating into flee under heavier pressure.
- Break line of sight behind cover and confirm the lookout stops going straight back into immediate fire while the world shows a search marker around your last seen position.
- Enter a staged vehicle after pressure starts, drive a short distance, then exit back near the lane and confirm the search marker and HUD both preserve the hostile search or reacquire state instead of snapping straight from vehicle escape back into fire.
- Keep moving long enough for the witness to cool down and confirm the street card reports that the block is settling instead of staying permanently escalated.

## Blockers and risks

- The witness is a single authored actor, so this cycle proves the loop but does not yet establish how multiple civilians or broader district populations will react together.
- The hostile search state improves readability and pacing, but it is still not a full pursuit or consequence model. There is no wider law, faction, or neighborhood escalation handoff yet.
- The street HUD is clearer than before, but mixed systemic states are still only represented through lightweight cards and world markers. A fuller UX pass is still ahead once more actors and consequences exist.

## Next cycle goal

Cycle 21 should deepen the systemic slice by adding:

- another pass that lets witness pressure propagate more credibly into nearby street activity or additional civilian feedback instead of stopping at one actor
- a stronger bridge between hostile search, reacquire, and recovery so the lane produces a more legible rhythm of pressure, escape, and reset
- another HUD and world-feedback pass that keeps mixed combat, witness, and traffic states readable as the slice gets busier
- another integration check across on-foot combat, staged vehicle escape, witness response, and traffic hazard so the vertical slice keeps moving toward the Cycle 24 milestone
