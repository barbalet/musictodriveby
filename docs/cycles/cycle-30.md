# Cycle 30 Report

Date completed: April 17, 2026

## Goal

Add one more small layer of authored territory pressure without turning these actors into full AI:

- give the sidewalk edge a more readable intimidation beat so the runner can briefly challenge or screen the entry before full combat pressure starts
- let the runner and inner post use a few more staged offsets or stance changes so repeated approaches do not always silhouette the same way
- keep the lookout as the real combat threat and preserve the lean HUD approach by putting most of the new value into world motion and timing

## What shipped

- `Sources/EngineCore/include/engine_core.h` and `Sources/EngineCore/engine_core.c` now add a dedicated `screen` territory state for the sidewalk runner. The runner can now step across the lane mouth before full handoff, pinch the line briefly on both foot and vehicle approaches, and then fall through into the existing runner-to-post handoff once the player commits deeper or the block gets provoked.
- `Sources/EngineCore/engine_core.c` also gives both territory bodies a few more staged pockets instead of one fixed line. The runner now uses different watch, screen, and handoff offsets across the lane mouth, while the inner post supports from a small set of shifted receive pockets deeper inside the block. Hot returns and vehicle entries bias those offsets differently, so repeated approaches do not always frame the same two-body silhouette.
- `Sources/EngineCore/engine_core.c` threads that edge screen lightly into the lookout logic. A live runner screen now contributes a small amount to the lookout’s alert floor, attack range, and required-alert threshold, which helps the lane feel socially pressurized without moving the actual combat role away from the lookout.
- `Sources/MusicToDriveBy/Renderer.swift` now renders the new screen beat with stronger world posture, a wider screening cue on the runner, and more stateful inner-post presentation. The HUD, prompt, street-card, and debug language only switch into screen wording when the runner is actually pinching the entry.

## Notes on implementation

- This is still staged territory presence, not general gang AI. The runner and inner post remain lightweight authored bodies whose value comes from timing, spacing, and readable escalation.
- The important change is the new middle beat between idle edge presence and deeper handoff. The lane mouth now has a brief social challenge before the encounter collapses into the older claimed-territory flow.
- The renderer stays intentionally lean. The extra readability mostly comes from where the bodies stand and how they frame the line, with only a small amount of new text to confirm what the world is already showing.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk toward the lane mouth on foot and confirm the court-set sidewalk runner still idles outside the line as before.
- Step a little closer without fully committing and confirm the runner now screens the lane mouth before falling back into watch or handoff.
- Roll a staged vehicle along that same edge and confirm the runner screens the vehicle with different timing and wording from the on-foot approach.
- Step across the line and confirm the runner screen now gives way to the claimed-territory handoff instead of skipping straight from idle edge presence to generic deeper pressure.
- Push farther into claimed turf and confirm the inner post still takes the handoff, but now does so from slightly different staged pockets instead of one fixed deeper silhouette.
- Back off after the handoff is live, then re-enter while the block is still hot, and confirm the runner/post pair recover in a readable sequence while the edge screen still folds cleanly back into the faster hot-territory reception.

## Blockers and risks

- The edge now has a clearer intimidation beat, but the runner is still a staged body rather than a true faction NPC with navigation, dialogue, or bespoke reactions.
- The added screen pressure lightly helps the lookout, so live tuning is still needed to keep the lane mouth tense without making the slice feel over-scripted or unfair.
- The body staging is richer than before, but it still depends on a small authored set of pockets and timings rather than broader territory simulation.

## Next cycle goal

Cycle 31 should sharpen the social handoff at the lane mouth without broadening into full AI:

- add a lightweight retreat-or-clear-line beat so the runner does not hold the same screening posture once the inner post or lookout fully owns the pressure
- let the runner and inner post briefly re-face or clamp the entry from complementary angles when the player hesitates at the boundary
- keep the HUD lean and put most of the value into world spacing, timing, and readable transitions between screen, handoff, and cooldown
