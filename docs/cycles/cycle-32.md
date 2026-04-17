# Cycle 32 Report

Date completed: April 17, 2026

## Goal

Make the lane mouth recover and reform more gracefully without broadening into full AI:

- add a lightweight re-approach beat so the runner can return from clear into watch or screen when the player feints in and back out
- let the inner post ease forward or relax back during that retake so clamp and handoff do not snap off at one threshold
- keep the HUD lean and put most of the value into world motion and timing between clear, reform, and cooldown

## What shipped

- `Sources/EngineCore/include/engine_core.h` and `Sources/EngineCore/engine_core.c` now add a dedicated `reform` territory state for the sidewalk runner and inner post. After the runner clears out, backing off to the boundary now lets the line rebuild through a short retake instead of snapping straight from clear to idle.
- `Sources/EngineCore/engine_core.c` threads that reform state through both the claimed-edge and outside-line cases. The runner can reform from the clear pocket back toward watch or screen, while the inner post eases between support and clamp pockets so the pair retakes the edge with motion instead of a threshold pop.
- `Sources/EngineCore/engine_core.c` also feeds a small amount of reform pressure back into the lookout rules, which lets hot re-entry and feint-back-in passes feel connected to the same territorial slice without making the runner or post the primary combat actor.
- `Sources/MusicToDriveBy/Renderer.swift` now names and renders that reform beat in world space and only surfaces reform wording in the HUD, prompt pill, and street card when the line is genuinely retaking after a clear.

## Notes on implementation

- The change is about transition quality, not system breadth. The lane mouth now has a middle beat between clear and cooldown, which makes hesitation and re-approach feel socially occupied instead of binary.
- The reform beat is intentionally lightweight. It is there to help the runner and inner post re-form the edge, not to replace the lookout or add a second combat loop.
- The most important read is motion: the runner returns from clear, the inner post eases back toward support or forward toward clamp, and the text layer simply confirms that the line is reforming.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk toward the lane mouth on foot and confirm the runner still screens and clamps the entry before the older clear-line beat takes over.
- Step across the line, let the runner clear, then back off toward the boundary and confirm the runner now reforms the line instead of dropping straight to idle.
- Roll a staged vehicle through that same clear-and-back-off sequence and confirm the reform timing reads differently for the vehicle than it does on foot.
- Watch the inner post during that retake and confirm it eases forward or relaxes back with the reform instead of snapping between clamp, handoff, and cooldown.
- Push deeper again after the reform and confirm the handoff back into clear/post pressure still reads cleanly once the block recommits.
- Repeat the pass after a hot return and confirm reform, hot re-entry, search, and staggered fallback still read as one connected territory loop.

## Blockers and risks

- The reform beat improves the transition ladder, but the system is still authored around a small set of pockets and thresholds rather than broader faction AI.
- Reform now touches both the runner and inner post, so threshold tuning still matters; if the blend windows are off, the retake could feel too twitchy or too soft.
- The HUD stays intentionally lean, which means most of the success criteria still depend on the in-motion read during live play rather than on text alone.

## Next cycle goal

Cycle 33 should make the retake resolve more deliberately once reform is active:

- add a lightweight settle-or-harden beat so reform can either resolve back into screen/watch or lock toward clamp depending on player commitment
- give the inner post a clearer completion rhythm so the retake does not feel equally strong at every distance from the boundary
- keep the presentation focused on body spacing and timing instead of expanding the UI
