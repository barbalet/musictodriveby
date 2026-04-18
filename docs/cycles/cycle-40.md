# Cycle 40 Report

Date completed: April 17, 2026

## Goal

Give the repeated reclaim-feint and retake-challenge edge games a little more positional answer instead of only timing and text changes:

- let reclaim feints counter-step more clearly at the mouth while the inner post turns the deeper follow-up angle
- let retake challenges cross-angle the lane mouth more clearly before the heavier recommit lands
- keep that motion readable through the existing runner, inner-post, lookout, prompt, and street cues rather than adding a new system or panel

## What shipped

- `Sources/EngineCore/engine_core.c` now gives reclaim-return feints a clearer positional bait. During repeated shallow edge games, the runner can step off the mouth while the inner post turns deeper, so the reclaim side feels like a soft opening that is actively dragging the next commit into the pocket.
- `Sources/EngineCore/engine_core.c` now gives retake-return challenges a clearer positional check. The runner and inner post can take complementary angles across the lane mouth before the heavier recommit, so the retake side reads as an actual cross-angle test instead of the same early brace with different text.
- `Sources/EngineCore/engine_core.c` also carries those small pose changes into the just-outside territory carry, so backing off and re-pressing the edge still preserves the same runner/post answer instead of flattening back into a neutral reform or brace.
- `Sources/MusicToDriveBy/Renderer.swift` now surfaces those live poses through the existing world marker, combat card, prompt pill, street card, and territory summary. `reclaim feint` can now call out the runner counter-step, while `edge challenge` can now call out the cross-angle when those poses are actually live.
- `README.md` now advances the repo to Cycle 41 and adds manual checks for the new counter-step and cross-angle behavior.

## Notes on implementation

- This pass still uses the existing patrol and inner-post actors, positions, headings, and state ladder. The new behavior comes from steering those existing bodies into different offsets and facing targets, not from adding a third territory actor or a new encounter state.
- Reclaim return now has a clearer soft-mouth bait: the runner can peel away from the player’s line while the post turns the deeper receive angle.
- Retake return now has a clearer hard-mouth check: the runner and post can cross-angle the mouth before the line settles into the heavier recommit.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Finish a held-pocket path, let the reclaim settle, then re-approach quickly.
- Hover at the lane mouth on that reclaim-return pass, back off, and test it again in a shallow loop. Confirm the runner now counter-steps off the mouth while the inner post turns the next angle deeper, and the prompt/card still read that as a reclaim feint.
- Push deeper after that reclaim-side counter-step and confirm the softer mouth still flips cleanly into a deeper reclaim commit.
- Finish a clean pullout, let the short edge-retake settle, then turn back in quickly.
- Test the lane mouth without fully driving through it, back off, and press it again. Confirm the runner and inner post now cross-angle the mouth during the repeated retake challenge before the heavier recommit.
- Push through that cross-angle challenge and confirm it still resolves into a clear retake commit instead of hanging in a static brace.
- Check the world marker, prompt pill, combat card, street card, and debug territory summary together and confirm they agree on the counter-step reclaim feint versus the cross-angle edge challenge.
- Repeat at least one reclaim-side pass and one retake-side pass in a staged vehicle and confirm the same positional answers still read there.

## Blockers and risks

- The new positional answer is still local and threshold-driven. It does not yet build a broader memory of which side the player keeps favoring across multiple returns.
- The new side-step and cross-angle may read too subtly or too noisily depending on lateral approach and camera framing. This still needs a real in-app feel pass.
- The renderer is inferring these reads from live pose and heading, so if tuning drifts the labels could stop matching what the player actually feels.

## Next cycle goal

Cycle 41 should start letting the repeated edge game remember which side of the mouth the player keeps favoring:

- let reclaim and retake responses bias the preferred handoff side based on repeated left/right approach instead of only generic lateral offset
- keep that bias readable through the same runner, post, lookout, prompt, and street cues rather than adding new UI or mission logic
- continue deepening the local faction-territory exchange before the broader mission and world-content frameworks arrive
