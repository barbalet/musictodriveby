# Cycle 7 Report

Date completed: April 16, 2026

## Goal

Build on the first road-graph and activation pass by adding:

- a third connected authored block so the graph stops being a simple two-node line
- lightweight spawn lifecycle rules so placeholder population appears and clears with more intent
- a cleaner split between static world geometry and activation-driven dynamic scene content
- early district and block-tag authoring conventions that later mission, gang, and population systems can reuse

## What shipped

- `Sources/EngineCore` now authors a third mixed-use branch block east of the hub, with a second explicit road link so the graph branches instead of staying purely north-south.
- Block descriptors now export district identity and tag masks, which gives the world layout a first reusable authoring vocabulary for neighborhood flavor and future systemic ownership.
- The engine now exports a dedicated dynamic-prop stream for activation-driven accents such as signal lamps, swing signs, pennants, transit glows, and neon strips instead of forcing the renderer to infer those props from block kind alone.
- `Sources/MusicToDriveBy/Renderer.swift` now reads that dynamic-prop metadata, animates it per prop type, and reports the richer authored layout in the debug overlay.
- Placeholder pedestrian and vehicle hooks now use lightweight live-window rules based on block identity and time, so nearby population fades in and out with more intention instead of appearing on every eligible hook all the time.

## Notes on implementation

- The new branch block sits on the east-west road that already crossed the hub, which keeps the graybox road model simple while still turning the authored graph into a true branch.
- Static graybox geometry is still exported as one box scene, but authored ambient motion now lives in its own exported metadata layer, which is a cleaner seam for future activation and streaming work.
- District and tag metadata currently drive overlay labels, dynamic-prop identity, and placeholder timing and tinting; they do not yet feed missions, factions, or true population simulation.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Open `MusicToDriveBy.xcodeproj` and run the `MusicToDriveBy` scheme on `My Mac` if you want the Xcode app flow.
- Walk north to the residential block and east along the branch to the new mixed-use block.
- Confirm the overlay shows three blocks, two links, dynamic-prop counts, the active district/tag summary, and live pedestrian and vehicle counts that change over time.
- Verify the mixed-use branch shows its own animated storefront accents and that live placeholder population appears and clears in waves instead of staying permanently active.

## Blockers and risks

- The world still renders from a single static scene-box export, so true geometry streaming or per-block static activation has not landed yet.
- Spawn lifecycle rules are intentionally lightweight and renderer-side; they are not backed by collision, AI, traffic logic, or persistent world ownership.
- District and tag metadata are authored by hand and still small in scope, so the conventions may need revision once factions, missions, and larger districts arrive.
- The branch block makes the graph meaningfully less linear, but the surrounding road system is still tiny and heavily hand-authored.

## Next cycle goal

Cycle 8 should deepen the world-layout foundation by adding:

- a first per-block static-scene ownership split so activation affects more than the dynamic metadata layer
- another authored connector, side street, or service lane so the branching graph starts to feel navigable instead of symbolic
- stronger district-specific ambient rules that push the new tag and district data beyond overlay/debug use
- a first step toward reusable population profiles so future pedestrians, vehicles, gangs, and missions can consume the same block identity data
