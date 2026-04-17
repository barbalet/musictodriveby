# Cycle 21 Report

Date completed: April 17, 2026

## Goal

Deepen the systemic slice by adding:

- another pass that lets witness pressure propagate more credibly into nearby street activity or additional civilian feedback instead of stopping at one actor
- a stronger bridge between hostile search, reacquire, and recovery so the lane produces a more legible rhythm of pressure, escape, and reset
- another HUD and world-feedback pass that keeps mixed combat, witness, and traffic states readable as the slice gets busier
- another integration check across on-foot combat, staged vehicle escape, witness response, and traffic hazard so the vertical slice keeps moving toward the Cycle 24 milestone

## What shipped

- `Sources/EngineCore/include/engine_core.h` and `Sources/EngineCore/engine_core.c` now export a persistent street-incident layer on top of the witness/search logic. Gunfire, blocked shots, hostile bursts, and vehicle escape can all raise a local incident state with a retained position, level, and decay timer instead of leaving systemic spillover as a momentary side effect.
- `Sources/EngineCore/engine_core.c` now uses that incident layer to push extra traffic occupancies into the nearby road network. Ambient traffic therefore eases back from the combat block once the witness flees or the lookout search spreads into the street, giving the lane a visible effect on the surrounding slice instead of keeping vehicle flow completely disconnected from combat.
- `Sources/EngineCore/engine_core.c` also tightens the hostile rhythm around broken line of sight. Search states now feed a short settle window before direct fire resumes, and recovery delay is shortened slightly while the player holds a real search window in cover or after vehicle escape, making pressure, recovery, and reacquire read as one loop instead of three unrelated timers.
- `Sources/MusicToDriveBy/Renderer.swift` now reflects that broader street response in both rendering and HUD logic. Ambient vehicles react more strongly to incident occupancies, the world gets a separate incident marker in addition to the hostile search marker, the combat summary now reports incident intensity, and the street card distinguishes between calm, search, cooling, holding, and full incident states while accounting for traffic pressure.

## Notes on implementation

- Traffic is still reacting through occupancy pressure, not through fully simulated civilian drivers. That keeps the current slice lightweight while still proving that combat can influence the broader street.
- The new street-incident layer is intentionally local to the combat pocket. It is a bridge toward wider systemic consequence, not yet a full district alert or law-response system.
- Search-to-reacquire is still authored, but it is now easier to read because the lookout, player recovery window, street marker, and HUD all stay in phase for a little longer after line of sight breaks.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk north into the combat lane, grab the lead pipe or pistol with `T`, and confirm the HUD still shows separate health, weapon, encounter, street, and prompt panels.
- Trigger the witness with gunfire or a close melee hit and confirm the street marker appears in addition to the witness state change instead of leaving the reaction only in the debug text.
- Break line of sight behind cover and confirm the lookout searches the last seen position, then settles through a short reacquire window before returning to live fire.
- Escape into a staged vehicle after pressure starts, drive a short distance, then exit back near the lane and confirm the search state, street marker, and reacquire timing stay coherent together.
- Watch the nearby road once the witness has fled and confirm ambient traffic now gives the combat block more space while the incident is live.

## Blockers and risks

- Traffic now reacts to the combat block, but only through occupancy pressure. There is still no explicit civilian driver behavior, no honking or avoidance animation layer, and no broader district propagation.
- The street incident is local and temporary, so it improves readability without yet proving how multiple witnesses or parallel incidents should stack.
- The HUD is carrying more mixed-state information well enough for prototyping, but there is still no minimap, reticle pass, or dedicated consequence UI beyond the current cards and world markers.

## Next cycle goal

Cycle 22 should keep building toward the Cycle 24 vertical-slice milestone by adding:

- another civilian-feedback pass so the street feels populated by more than one authored reaction source, even if that still uses lightweight placeholder logic
- another pass on incident spread and recovery so witness response, hostile search, and traffic easing can scale without becoming noisy or unreadable
- another HUD and world-feedback refinement that keeps combat pressure, street incident state, and traversal prompts legible when they overlap
- another integration check across lane combat, staged vehicle escape, traffic reaction, and broader ambient population activity so the systemic slice keeps converging on a believable first vertical slice
