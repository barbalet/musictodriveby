# Cycle 14 Report

Date completed: April 17, 2026

## Goal

Close the vehicle-fundamentals phase by adding:

- lane-hold and stop-zone rules that make ambient traffic pause more deliberately at shared chokepoints and intersections
- manual handoff candidate stepping or lock confirmation so dense curbside clusters remain predictable under player input
- tighter bicycle and motorcycle edge-case polish, especially around curb hops, low-speed turning, and dismount placement
- a final vehicle-fundamentals cleanup pass that resolves obvious roster inconsistencies before the project moves into combat foundations

## What shipped

- `Sources/EngineCore/include/engine_core.h`, `Sources/EngineCore/engine_core.c`, and `Sources/MusicToDriveBy/InputController.swift` now support manual staged-vehicle control. On-foot play has explicit cycle and lock inputs, the engine preserves a locked candidate when possible, and the current preview stack can be rotated without losing the ranked handoff context.
- `Sources/MusicToDriveBy/Renderer.swift` and `Sources/MusicToDriveBy/GameViewModel.swift` now surface that control state clearly. The overlay lists `R` and `G`, the prompt explains when to cycle or lock, the handoff debug line marks selected and locked candidates, and locked staged vehicles render with a stronger world marker so curbside clusters stay readable.
- Ambient placeholder traffic now treats engine stop zones more like short holds than generic blocker pressure. Stop-zone occupancies pull harder on route progress, raise brake intensity sooner, and make traffic pause more deliberately at the active road link instead of only drifting backward under generic occupancy pressure.
- Bicycle, moped, and motorcycle movement got a tighter edge-case pass in the engine. Two-wheel vehicles now turn more willingly at low speed, receive lighter lane-assist pull while maneuvering slowly, smooth short curb or lot transitions more gently, and dismount toward sidewalk- or lot-friendly placements when space allows.
- This cycle also completed the final vehicle-fundamentals cleanup pass for the current slice: the staged roster, prompt language, lock state, recovery readouts, and traffic-pressure hooks now feel coherent enough to hand off into the combat-foundations phase instead of still reading as disconnected prototype experiments.

## Notes on implementation

- The new stop-zone behavior is still placeholder traffic logic, not a true lane queue or right-of-way simulation. Vehicles pause more intentionally, but they are not yet agents negotiating authored signs, signals, or intersection order.
- Handoff locking is intentionally lightweight. It stabilizes the selected vehicle under player input and current preview-radius churn, but it does not yet survive across large movement changes, streaming changes, or mounting another vehicle.
- Two-wheel curb polish softens low-speed edge cases without introducing a full suspension, rider animation, or physics model. The feel is more forgiving, but still graybox and tightly scoped.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Approach two or three staged vehicles in the same area and confirm `R` rotates the selected handoff candidate while the prompt and marker stack follow the new choice.
- Press `G` while on foot near a staged vehicle and confirm the handoff line reports a locked choice and the corresponding marker gains the stronger locked presentation.
- Stand on or drive through an active road link and confirm ambient placeholder traffic now holds more deliberately around the exported stop-zone pressure instead of only gliding backward under generic occupancy rules.
- Ride the bicycle, moped, and motorcycle over curbs and into slow turns, then confirm those classes pivot more willingly at low speed and recover more softly when transitioning onto curb, sidewalk, or lot surfaces.
- Dismount a bicycle or motorcycle near a curb edge and confirm the exit placement prefers a safer sidewalk or lot-side landing when one is available.

## Blockers and risks

- Traffic still lacks true queueing, intersection negotiation, and systemic right-of-way ownership.
- Handoff lock state is useful locally, but there is still no broader HUD, iconography, or persistence model for vehicle choice once the game moves beyond this small slice.
- Two-wheel handling is now more forgiving, but there is still no rider animation, suspension model, damage state, or per-class audio response.
- Vehicle-fundamentals work is now coherent enough to move on, but combat systems will stress the current overlay, input model, and simulation clarity quickly if they are not introduced carefully.

## Next cycle goal

Cycle 15 should begin combat foundations by adding:

- a first melee-weapon scaffold with pickup, equip, and readable close-range attack timing
- a simple damage target or reaction pass so hits produce visible consequences instead of only traversal-state changes
- the first combat-facing overlay or prompt updates needed to keep weapon state readable without losing the current vehicle and traversal debug value
- a narrow integration check proving the project can keep on-foot combat groundwork compatible with the existing vehicle traversal slice
