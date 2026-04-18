# Cycle 38 Report

Date completed: April 17, 2026

## Goal

Turn the new reclaim-return and retake-return memory into a slightly richer lane-mouth exchange instead of only a timing bias:

- let reclaim returns create a softer edge feint before the pocket commits deeper
- let retake returns create an earlier edge challenge before the block recommits through the mouth
- keep that texture inside the existing runner, inner-post, lookout, prompt, and street language

## What shipped

- `Sources/EngineCore/engine_core.c` now gives reclaim-return passes a short edge-feint branch at the boundary. The runner can show a softer screen at the lane mouth while the inner post leans deeper, so the next pass feels like a baited pocket instead of only a faster timer.
- `Sources/EngineCore/engine_core.c` now gives retake-return passes a short edge-challenge branch before full hardening. The runner and post can check the return earlier at the mouth, then resolve into a stronger recommit if the player keeps pushing instead of just brushing the edge.
- `Sources/EngineCore/engine_core.c` still reuses the existing patrol and inner-post ladder for all of this. The new texture is created by steering screen, brace, handoff, and support states differently, not by adding a separate encounter stack.
- `Sources/MusicToDriveBy/Renderer.swift` now surfaces those beats through the existing world marker, combat card, prompt pill, street card, and territory summary as `reclaim feint`, `reclaim commit`, `edge challenge`, and `retake commit`.
- `README.md` now advances the repo to Cycle 39 and adds manual checks for the new reclaim-feint and edge-challenge beats.

## Notes on implementation

- Reclaim return is now a two-step read: the edge can look softer for a moment, but that softness is carrying deeper pressure rather than real calm.
- Retake return is now a sharper two-step read: the lane mouth checks the player sooner, and only if the player commits does that challenge fold into the heavier recommit.
- The renderer changes stay inside the existing presentation stack. There is still no separate mission panel or special-purpose objective widget for these beats.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Finish a held-pocket path, let the reclaim settle, then re-approach quickly.
- Hover at the lane mouth on that reclaim-return pass and confirm the block now reads as a softer reclaim feint instead of jumping straight to the old recommit.
- Push deeper on that same reclaim-return pass and confirm the softer mouth now flips into a faster deeper pocket commit.
- Finish a clean pullout, let the short edge-retake settle, then turn back in quickly.
- Test the lane mouth without fully driving through it and confirm the block now throws an earlier edge challenge before the heavier recommit.
- Push through that challenge and confirm it now flips into a clearer recommit through the mouth instead of staying in one static brace.
- Check the world marker, prompt pill, combat card, street card, and debug territory summary together and confirm they agree on `reclaim feint`, `reclaim commit`, `edge challenge`, and `retake commit`.
- Repeat at least one of those passes in a staged vehicle and confirm the same feint-versus-challenge distinction still reads cleanly there.

## Blockers and risks

- The new return textures are still short and local. They do not yet build into a broader faction memory or repeated-pass adaptation.
- The distinction still depends heavily on timing and threshold tuning. If the patrol or inner-post alerts drift, the feint and challenge could collapse back toward generic screen or brace behavior.
- This still needs an in-app feel pass to verify that the new labels match what the player actually feels at the lane mouth.

## Next cycle goal

Cycle 39 should start making those edge beats answer repeated half-commits a little more actively:

- let the feint and challenge react more clearly to repeated shallow recommits or backing-off loops
- keep that response readable through existing runner, post, lookout, and street cues rather than new UI layers
- continue deepening the local territory exchange before the broader mission and world-content frameworks arrive
