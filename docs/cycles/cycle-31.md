# Cycle 31 Report

Date completed: April 17, 2026

## Goal

Sharpen the social handoff at the lane mouth without broadening into full AI:

- add a lightweight retreat-or-clear-line beat so the runner does not hold the same screening posture once the inner post or lookout fully owns the pressure
- let the runner and inner post briefly re-face or clamp the entry from complementary angles when the player hesitates at the boundary
- keep the HUD lean and put most of the value into world spacing, timing, and readable transitions between screen, handoff, and cooldown

## What shipped

- `Sources/EngineCore/include/engine_core.h` and `Sources/EngineCore/engine_core.c` now add a dedicated `clear` territory state for the sidewalk runner. Once the inner post or lookout fully owns the lane, the runner now steps out of the screening line instead of holding the same posture, which makes the handoff read like a staged retreat rather than a frozen blocker.
- `Sources/EngineCore/engine_core.c` also adds a brief boundary-clamp beat. If the player hesitates at the line, the runner and inner post now shift into complementary positions and re-face the entry from opposite angles before the encounter resolves into a deeper handoff or backs back down.
- `Sources/EngineCore/engine_core.c` keeps that pressure lightweight but systemic. The clamp and clear-line timing feed the existing front/deep watch values, while the older lookout rules still own the real combat threat once the line breaks open.
- `Sources/MusicToDriveBy/Renderer.swift` now makes those transitions readable in world space with a distinct clear-line runner posture, stronger clamp corridor cues, and tighter HUD, prompt, street-card, and debug wording that only switches into clamp or clear language when those beats are actually live.

## Notes on implementation

- The important change is transition quality, not system breadth. The runner now has somewhere to go after screening, and the paired bodies can briefly frame the boundary before deciding whether to hand inward or cool back off.
- The clamp is intentionally short-lived. It is there to give hesitation at the line a social answer, not to create a second combat gate on top of the lookout.
- The HUD stays narrow. Most of the readability comes from the bodies re-spacing and re-facing the boundary, with text only confirming whether the line is being screened, clamped, cleared, or handed inward.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk toward the lane mouth on foot and confirm the court-set sidewalk runner still idles outside the line as before.
- Step a little closer without fully committing and confirm the runner screens the lane mouth first, then briefly clamps it with the inner post from the opposite side if you hesitate at the boundary.
- Roll a staged vehicle along that same edge and confirm the clamp reads differently for the vehicle than it does on foot before the deeper handoff takes over.
- Step across the line and confirm the runner no longer holds the same screening pose once the inner post or lookout owns the pressure. The runner should clear out of the line instead of freezing in the lane mouth.
- Push farther into claimed turf and confirm the runner now clears the line while the inner post owns the pocket, with the encounter readout shifting into clear/post language once the handoff is live.
- Back off again and confirm the older staggered fallback, search, reopening, and hot re-entry behavior still reads cleanly with the new clamp and clear-line transitions in place.

## Blockers and risks

- The lane mouth now transitions more cleanly, but the runner and inner post are still lightweight staged bodies rather than general-purpose faction NPCs.
- The clamp and clear-line beats add more authored timing at the boundary, so live tuning is still needed to keep the line tense without making it feel over-scripted.
- The system now has a better transition ladder, but it still relies on a small set of authored pockets and thresholds rather than broader faction simulation.

## Next cycle goal

Cycle 32 should make the lane mouth recover and reform more gracefully without adding full AI:

- add a lightweight re-approach beat so the runner can return from clear into watch or screen when the player feints in and back out
- let the inner post ease forward or relax back during that retake so clamp and handoff do not snap off at one threshold
- keep the HUD lean and put most of the value into world motion and timing between clear, reform, and cooldown
