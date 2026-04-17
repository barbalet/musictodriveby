# Cycle 8 Report

Date completed: April 16, 2026

## Goal

Deepen the world-layout foundation by adding:

- a first per-block static-scene ownership split so activation affects more than the dynamic metadata layer
- another authored connector, side street, or service lane so the branching graph starts to feel navigable instead of symbolic
- stronger district-specific ambient rules that push the new tag and district data beyond overlay and debug use
- a first reusable population-profile pass so future pedestrians, vehicles, gangs, and missions can consume the same block identity data

## What shipped

- `Sources/EngineCore` now authors a fourth northeast block and a second north-south corridor at `x = 96`, turning the road graph into a small four-link grid instead of a three-block branch.
- Static scene export now carries ownership metadata, which lets the renderer keep shared roads and markings visible while culling block-owned lots, buildings, frontage props, and service-court dressing outside the active neighborhood slice.
- The northeast Market Spur block now includes a service-court annex with a loading lane, staging props, extra spawn hooks, and its own accent lighting so the new connector reads like a place rather than just a graph node.
- The engine now exports authored population profiles per block, and `Sources/MusicToDriveBy/Renderer.swift` consumes those profiles to drive placeholder pedestrian density, vehicle activity, travel span, and district ambient energy instead of hardcoding all of that behavior renderer-side.
- The renderer now layers district and tag-driven ambient accents on active blocks, reports live static-scene visibility in the overlay, and keeps the shared grid roads readable while nearby blocks stream in with more intent.

## Notes on implementation

- This is intentionally a first ownership split, not full geometry streaming. Shared roads, sidewalks, and markings still stay resident, while authored block content is what now comes and goes with activation.
- Population profiles are currently consumed by placeholder pedestrians, placeholder vehicles, and the ambient accent pass. They are authored engine data now, but they are not yet feeding AI, mission logic, or persistence.
- The new northeast block reuses the mixed-use family but adds a service-court variant and extra authored metadata so the new corner feels different from the earlier Market Spur block.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk east to the Market Spur corridor and then north into the new northeast block.
- Confirm the overlay reports four blocks, four links, live static visibility counts, and a population-profile summary for the active block.
- Verify that distant block-owned buildings and lot dressing fall away while the shared road grid remains visible.
- Check that the northeast service court shows its loading-lane accents and that placeholder pedestrian and vehicle behavior feels denser and more traffic-heavy in the Market Spur blocks than in Maple Heights.

## Blockers and risks

- Static activation is still a renderer submission split, not true memory-backed geometry streaming or unload and reload behavior.
- The new northeast block adds navigability, but mixed-use content authoring is still partly hand-built and partly variant-based, so duplication risk remains as the map grows.
- Population profiles are reusable metadata now, but they still drive only visual placeholder systems rather than real NPC, gang, traffic, or mission logic.
- Active-link evaluation still highlights only the closest qualifying road link, which is enough for this slice but not yet a route or traffic-state system.

## Next cycle goal

Cycle 9 should build on this grid and ownership pass by adding:

- first block-family or frontage-template variation rules so new authored blocks stop relying on near-duplicate hand layout
- link-aware placeholder movement across the four-block graph so pedestrians and vehicles start to read like they belong to routes instead of isolated spawn loops
- stronger chunk or ownership identifiers that move the current render-side cull toward true streaming boundaries
- another consumer for authored population profiles or block identity data, ideally a first mission, gang-territory, or hotspot hook
