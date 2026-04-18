# Cycle 44 Report

Date completed: April 17, 2026

## Goal

Let the remembered territory shoulder survive the cooldown path instead of only steering the live reclaim or reseal:

- let normalization, reopening, and hot re-entry cues remember which shoulder the last reclaim or reseal favored
- keep that cooldown carry readable through the existing street card, prompt, lookout summary, and territory summary cues rather than adding a new system
- continue deepening the local faction-territory exchange before the broader mission and world-content frameworks arrive

## What shipped

- `Sources/EngineCore/engine_core.c` now keeps `territory_preferred_side` alive while the street is still actively normalizing or reopening, so the remembered shoulder no longer gets cleared as soon as the direct watch and heat values fall off.
- `Sources/EngineCore/engine_core.c` also now biases the post-search settle incident toward the remembered shoulder, which lets the later recovery/reopening handoff inherit the same left or right side instead of snapping back to a neutral center.
- `Sources/MusicToDriveBy/Renderer.swift` now calls that cooldown carry out through the existing prompt pill, lookout summary, street/system card, and debug territory summary. Normalizing, reopening, and hot-territory readouts can now mention the remembered shoulder when it is still strong enough to matter.
- `README.md` now advances the repo to Cycle 45 and adds manual checks for shoulder-led normalization, reopening, and hot re-entry.

## Notes on implementation

- This pass still uses the same `territory_preferred_side` memory from the earlier territory cycles. It does not add a separate cooldown tracker.
- The engine change is mostly persistence and positioning work: preserve the shoulder while street incident or recovery timers are still materially active, then bias the settle incident so reopening follows the same side.
- The renderer stays lightweight and reuses the current HUD/prompt/summary surfaces instead of adding new cooldown widgets.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Finish a same-side held-pocket reclaim and stay near the block until the live watch settles. Confirm the street card and territory summary keep the same shoulder through normalization instead of dropping back to neutral as soon as the direct pressure clears.
- Finish a same-side clean pullout and confirm the lookout summary plus street card still tag that same shoulder as the street reopens instead of treating reopening as a generic cooldown.
- Re-enter during that reopening window from the same side and confirm the prompt and street card now call out that shoulder as the fast hot-territory read instead of falling back to generic burned-block wording.
- Swap sides on a later pass and confirm the cooldown/reopening/hot re-entry language migrates with the new remembered shoulder instead of sticking to the previous one.

## Blockers and risks

- The cooldown carry still depends on the existing preferred-side bias, so near-center exits or re-entries can still read intentionally neutral.
- Reopening still expresses itself through the existing street incident/recovery layer rather than through a richer civilian or traffic reform simulation.
- The lookout summary only adds cooldown shoulder tags when the current anchor actually matches the remembered shoulder, so some cooldown passes will still look quieter by design.

## Next cycle goal

Cycle 45 should start letting that remembered shoulder shape where the post-cooldown block reforms instead of only how it is described:

- let reopening and post-reopen spillback start biasing which shoulder nearby traffic, sidewalk pressure, or boundary reform language gathers around
- keep that reform readable through the existing street card, prompt, lookout summary, and territory summary cues rather than adding a new system
- continue deepening the local faction-territory exchange before the broader mission and world-content frameworks arrive
