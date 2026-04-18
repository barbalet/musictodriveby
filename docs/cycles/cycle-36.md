# Cycle 36 Report

Date completed: April 17, 2026

## Goal

Give the two post-break outcomes more distinct short-term territory consequences without widening into full mission scripting:

- let a held pocket produce a clearer short reclaim deeper in the block
- let a clean pullout produce a clearer short lane-mouth retake or reseal
- keep both consequences readable through the existing runner, inner-post, lookout, and street cues

## What shipped

- `Sources/EngineCore/engine_core.c` now gives a completed held-pocket outcome a short deeper reclaim bias instead of letting it decay into generic claimed-territory pressure. During that aftermath the runner and inner post stay oriented around the reclaimed pocket longer, deep watch carries harder, and hostile alert floors stay slightly hotter.
- `Sources/EngineCore/engine_core.c` now gives a completed clean-pullout outcome a short edge-retake bias instead of treating it like the same generic settle. The runner and post now reseal the lane mouth more deliberately while lookout exit-search and street recovery stay attached to that edge answer.
- `Sources/MusicToDriveBy/Renderer.swift` now reframes those aftermaths in the existing UI as `pocket reclaim` versus `edge retake`, so the combat card, prompt pill, street card, and territory summary all acknowledge the different short-term consequence rather than only the initial resolve choice.
- `README.md` now advances the repo to Cycle 37 and adds manual checks for the new deeper reclaim versus lane-mouth retake aftermaths.

## Notes on implementation

- This cycle intentionally reuses the existing patrol and inner-post states instead of inventing a second behavior stack. The runner/post pair still uses screen, reform, brace, clear, and handoff, but the short aftermath bias now pushes those states differently depending on how the player finished the break.
- The held-pocket path now keeps more energy deeper in the block, which makes the aftermath feel like the court-set is reclaiming interior ground instead of only re-closing the line.
- The clean-pullout path now resolves closer to the boundary, which makes the aftermath feel like a resealed mouth and exit read rather than the same deeper reclaim with different text on top.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Trigger the existing screen, reform, brace, commit, and resolve chain until the line breaks.
- Stay inside through the hold path and confirm the aftermath now reads as a short deeper reclaim rather than generic lingering claimed-turf pressure.
- Watch the runner/post/lookout mix during that held-pocket aftermath and confirm the energy stays deeper in the block instead of immediately collapsing back to the lane mouth.
- Break the line again, pull out cleanly, and confirm the aftermath now reads as a short edge retake or reseal rather than the same reclaim behavior.
- Watch the lane mouth during that clean-pullout aftermath and confirm the runner and post answer at the boundary while lookout exit-search and street recovery stay attached to that edge behavior.
- Repeat at least one of those aftermaths in a staged vehicle and confirm the distinction still reads cleanly alongside the vehicle territory language.
- Confirm the prompt, encounter card, street card, and territory summary all call out reclaim versus retake without adding a separate objective or mission panel.

## Blockers and risks

- The territory aftermaths are clearer now, but they are still local and threshold-driven rather than being part of a wider faction simulation.
- Because both aftermaths reuse the same patrol/post state ladder, tuning still matters a lot; if the values drift, reclaim and retake could collapse back toward looking too similar.
- The feature is still carried heavily by timing, spacing, and state messaging, so it still needs a live feel pass in-app to know whether the distinction is strong enough.

## Next cycle goal

Cycle 37 should start making those aftermaths shape the player’s immediate next pass a little more without turning into full mission logic:

- let reclaim and retake bias the next short re-approach differently
- keep that bias readable through the existing runner/post/lookout/street cues rather than new UI
- continue deepening this local territory encounter ladder before the broader mission framework arrives
