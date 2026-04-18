# Cycle 37 Report

Date completed: April 17, 2026

## Goal

Make the two territory aftermaths shape the player’s immediate next pass a little more without widening into mission scripting:

- let a held-pocket reclaim bias the next short re-approach toward faster deeper pickup
- let a clean-pullout retake bias the next short re-approach toward an earlier lane-mouth answer
- keep that difference readable through the existing runner, inner-post, lookout, prompt, and street cues

## What shipped

- `Sources/EngineCore/include/engine_core.h` and `Sources/EngineCore/engine_core.c` now export and drive a lightweight territory re-approach memory with separate `reclaim` and `retake` modes plus a short timer.
- `Sources/EngineCore/engine_core.c` now feeds that memory back into the next pass: reclaim returns ease the edge and wake deeper handoff pressure sooner, while retake returns harden the lane mouth earlier and keep more front-watch pressure alive on the way back in.
- `Sources/EngineCore/engine_core.c` also lets that short re-approach memory influence lookout alerting and attack thresholds, so the next pass is not only a runner/post read but also a slightly different hostile pickup rhythm.
- `Sources/MusicToDriveBy/Renderer.swift` now surfaces that difference through the existing world marker, combat card, prompt pill, street card, and territory summary as `reclaim return` versus `retake return` instead of leaving the next pass to feel like a cold restart.
- `README.md` now advances the repo to Cycle 38 and adds manual checks for the new reclaim-return versus retake-return re-approach read.

## Notes on implementation

- The new memory is intentionally short and local. It does not create new encounter content or persistent faction state; it only biases the immediate follow-up pass after the previous resolve finishes.
- Reclaim returns lean on the existing handoff and deep-watch ladder rather than inventing a new deeper-state stack. The edge loosens slightly, but the post and lookout pick up the pocket sooner.
- Retake returns reuse the existing brace and clamp ladder. The block does not become globally hotter; it just meets the next pass at the lane mouth faster before giving depth.
- The renderer changes stay inside the current presentation surfaces. There is no new panel or objective widget for this beat.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Break the line and finish the held-pocket path until the reclaim aftermath lands.
- Let that resolve clear, then re-approach quickly and confirm the return now reads as reclaim memory rather than a cold start. The runner should feel lighter at the mouth while the inner post and lookout pick up the deeper pass sooner.
- Break the line again, finish the clean-pullout path, and let the short edge-retake aftermath settle.
- Re-approach quickly after that pullout and confirm the return now reads as retake memory rather than the same reclaim-biased pass. The runner and post should answer earlier at the lane mouth and the lookout should still feel attached to that edge answer.
- Check the world marker, prompt pill, encounter card, street card, and debug territory summary together and confirm they all agree on `reclaim return` versus `retake return` without opening a new mission or objective panel.
- Repeat at least one of those returns in a staged vehicle and confirm the earlier edge answer versus deeper pickup distinction still reads cleanly alongside the vehicle territory language.

## Blockers and risks

- The return memory is still intentionally short and local, so it does not yet accumulate across multiple passes or wider faction simulation.
- The reclaim-return and retake-return reads depend on timing and spatial feel. If the timers or alert weights drift, the two next-pass biases could collapse back toward feeling too similar.
- The renderer now labels the new beat, but the final call still needs an in-app feel pass to verify the world-space read is strong enough without over-explaining itself.

## Next cycle goal

Cycle 38 should start turning this into a slightly richer lane-mouth exchange rather than only a timing bias:

- let reclaim-return and retake-return passes create a more readable short feint versus commit texture at the edge
- keep that texture inside the existing runner, inner-post, lookout, and street language rather than opening full mission scripting
- continue deepening the local territory ladder before the broader world-content and mission frameworks take over
