# Cycle 41 Report

Date completed: April 17, 2026

## Goal

Let the repeated edge game remember which side of the lane mouth the player keeps favoring instead of only replaying the same counter-step or cross-angle off generic lateral offset:

- let reclaim and retake responses bias the preferred handoff side based on repeated left/right approach
- keep that bias readable through the existing runner, inner-post, prompt, street card, and territory summary rather than adding new UI or mission logic
- continue deepening the local faction-territory exchange before the broader mission and world-content frameworks arrive

## What shipped

- `Sources/EngineCore/include/engine_core.h` and `Sources/EngineCore/engine_core.c` now carry a lightweight remembered-side value for the territory loop. It updates around the lane mouth, survives the short re-approach beat, and decays only when the local territory thread fully cools.
- `Sources/EngineCore/engine_core.c` now blends that remembered side into the existing lateral bias for both the runner and the inner post. Reclaim-return feints and retake-return challenges can now keep favoring the same left or right shoulder across repeated shallow recommits instead of recomputing everything only from the current frame.
- `Sources/EngineCore/engine_core.c` also applies that remembered shoulder to the just-outside carry, so backing off and pressing the mouth again from the same side preserves the same runner/post answer instead of flattening back into a neutral lane-mouth staging.
- `Sources/MusicToDriveBy/Renderer.swift` now surfaces that shoulder memory through the existing combat HUD, prompt pill, street card, and territory summary as `left` or `right` shoulder language when the bias is actually strong enough to matter.
- `README.md` now advances the repo to Cycle 42 and adds manual checks for repeated same-side approaches and shoulder swapping.

## Notes on implementation

- The new memory is intentionally lightweight: a single signed float is enough to hold both direction and strength, while still decaying naturally when the territory thread ends.
- This pass still reuses the existing patrol and inner-post ladder. The new behavior comes from biasing the same target positions and heading focuses, not from adding a new territory state or another actor.
- The renderer is still reading live pose plus exported memory. There is still no separate shoulder-memory widget or new debug panel for this behavior.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Finish a held-pocket path, let the reclaim settle, then re-approach quickly from the same side of the lane mouth more than once.
- Confirm the reclaim-return feint now keeps favoring that same left or right shoulder: the runner should counter-step off it while the inner post turns the deeper angle there, and the prompt/card should call that shoulder out.
- Push deeper after that same-side reclaim loop and confirm the softer mouth still flips cleanly into a reclaim commit.
- Finish a clean pullout, let the short edge-retake settle, then re-press the lane mouth from the same side more than once.
- Confirm the retake-return challenge now keeps favoring that same left or right shoulder: the runner and post should cross-angle that remembered shoulder before the heavier recommit.
- Swap sides on a later reclaim or retake return and confirm the remembered shoulder can migrate once the repeated approach clearly changes side.
- Check the prompt pill, combat card, street card, and debug territory summary together and confirm they agree on the favored shoulder while those return beats are live.
- Repeat at least one reclaim-side pass and one retake-side pass in a staged vehicle and confirm the same remembered-shoulder behavior still reads there.

## Blockers and risks

- The new shoulder memory is still short and local. It does not yet persist across larger territory cooldowns or build a broader faction understanding of the player’s habits.
- The memory currently follows world-space left/right at the lane mouth. It still needs a feel pass to confirm that this reads naturally from the player camera in repeated on-foot and vehicle loops.
- Because the memory is blended into the same existing positions, subtle tuning changes could make the shoulder read feel too weak or too sticky.

## Next cycle goal

Cycle 42 should start letting that remembered shoulder affect deeper follow-through instead of only the lane mouth:

- let reclaim commits and retake recommits carry the favored shoulder farther into the handoff or reseal instead of dropping back to a more neutral pocket
- keep that follow-through readable through the same runner, post, lookout, prompt, and street cues rather than new systems
- continue deepening the local faction-territory exchange before the broader mission and world-content frameworks arrive
