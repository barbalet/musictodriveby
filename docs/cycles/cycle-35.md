# Cycle 35 Report

Date completed: April 17, 2026

## Goal

Turn the completed line break into a tiny follow-through decision without widening into a real mission framework:

- add a short resolve beat after the line breaks, with a readable hold-the-pocket path versus a pull-out-clean path
- let the existing HUD, prompt, and street card acknowledge that choice without adding a separate mission tracker
- keep the new beat tied to the same runner, inner-post, lookout, and street-pressure slice

## What shipped

- `Sources/EngineCore/include/engine_core.h` now exports `territory_resolve_state`, `territory_resolve_timer`, and `territory_resolve_progress`, plus a small resolve-state enum so the follow-through beat is readable outside the engine.
- `Sources/EngineCore/engine_core.c` now seeds a resolve window when the commit beat completes, then lets the player either stay deep enough to hold the pocket or back out through the lane mouth for a cleaner exit.
- `Sources/EngineCore/engine_core.c` also feeds those two outcomes back into the existing slice differently: holding the pocket carries more heat and lookout pressure deeper into claimed turf, while a clean pullout leans toward lookout exit-search and street recovery instead of pretending both choices are the same.
- `Sources/MusicToDriveBy/Renderer.swift` now extends the in-world objective marker into the resolve phase and threads the same state through the combat card, prompt pill, street card, and territory debug summary so hold versus pullout reads as one continuous local beat.
- `README.md` now advances the repo to Cycle 36 and adds manual checks for both follow-through outcomes after the line break.

## Notes on implementation

- This is still intentionally local. The new beat is not a general objective tree or mission system; it is a short post-break answer to the question of what the player does immediately after forcing the edge.
- The important change is consequence shape. A broken line no longer always resolves as the same short success tail. Staying in the pocket and peeling out now read as different answers, and they feed the block state differently.
- The renderer work stays compact on purpose. The prompt, encounter card, street card, and world marker all acknowledge the new decision, but there is still no separate log, tracker, or checklist panel.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Trigger the existing screen, reform, brace, and commit beats until the line breaks again.
- Stay inside after the break and confirm the new follow-through becomes a hold-the-pocket beat rather than falling straight back into generic claimed-turf language.
- Let that hold path complete and confirm the HUD and street card acknowledge a held pocket while pressure stays hotter inside the block.
- Break the line again, then pull back out through the lane mouth and confirm the resolve beat now flips into a distinct pullout path instead of sharing the same messaging as the hold path.
- Finish that pullout cleanly and confirm the lookout now reads your exit while the street card acknowledges the cleaner withdrawal and short settle window.
- Repeat at least one of those passes in a staged vehicle and confirm the same follow-through logic still reads cleanly alongside the vehicle territory language.
- Watch the runner/post/lookout/street handoff after both outcomes and confirm the feature still feels like one territory slice rather than a detached mini-mission.

## Blockers and risks

- The new decision is readable, but it is still heavily threshold-driven and local to one authored territory slice.
- Hold versus pullout now carries more behavioral meaning, so tuning drift could make one path feel much better or much clearer than the other.
- The presentation is still intentionally light, which means live motion and pacing still carry a lot of the feature’s readability burden.

## Next cycle goal

Cycle 36 should start giving those two follow-through outcomes slightly different world consequences without expanding into full mission scripting:

- let a held pocket and a clean pullout produce more distinct short-term territory recovery or reclaim behavior
- keep those consequences readable through the existing runner/post/lookout/street cues instead of adding new UI
- continue deepening this local territory slice before the broader mission framework arrives
