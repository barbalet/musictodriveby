# Cycle 59 Report

Date completed: April 18, 2026

## Goal

Push the city from structural realism toward surface realism by breaking up the too-clean frontage and parcel materials:

- give the shared storefront face a more believable material stack instead of one glass strip under one awning
- add wear and patching to the shared parking pads so the new lot kit stops looking freshly painted everywhere
- make repeated retail and parcel edges feel more used-in without giving up the scalability of the shared procedural kit

## What shipped

- `Sources/EngineCore/engine_core.c` now upgrades the shared storefront-awning helper with tile or kickplate bands, recessed center entries, transom bands, clearer mullions, and darker doorway depth so repeated shopfronts stop reading as one uninterrupted glass band.
- `Sources/EngineCore/engine_core.c` now upgrades the shared parking-pad helper with darker tire tracks, repair patches, faded stall paint variation, and oil-mark detail so the parcel asphalt no longer looks uniformly fresh.
- `Sources/EngineCore/engine_core.c` keeps that pass shared, so every frontage template using the awning kit and every block using the lot kit inherits the stronger material read without one-off per-block edits.
- `README.md` now advances the repo to Cycle 60, adds manual checks for the new frontage-material and pavement-wear pass, and updates the prototype summary to call out the used-in surface treatment.

## Notes on implementation

- This cycle stays within the current box-based renderer, but it shifts more attention from silhouette alone to how surfaces age, recess, and break up across repeated geometry.
- The storefront work intentionally stays inside the existing awning helper so mixed-use, hub, residential, and service-spur frontages all get the same material uplift automatically.
- The parking-wear pass uses only a handful of extra boxes per pad, which keeps the added realism broad without turning the lot system into a bespoke decal workflow.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk a retail or mixed-use frontage and confirm the storefronts now read with kickplates, recessed entries, transom bands, and clearer mullions instead of as one flat glass strip.
- Stand in several side-lot pads and confirm the asphalt now shows darker tire paths, repair patches, faded striping, and oil marks instead of one uniform clean surface.
- Compare multiple frontage templates and confirm the shared storefront material pass now supports the stronger awnings rather than leaving a simplified facade beneath them.
- Compare western and southern blocks and confirm the widened slice now feels more used-in at the surface level instead of like a freshly generated procedural kit.

## Blockers and risks

- These material changes still rely on layered box color treatment rather than decals or authored textures, so they improve perceived wear before the project reaches production-grade surfacing.
- The scene is getting denser across both structure and surface layers, so later full-map expansion should keep watching scene-box count and vertex pressure as more miles adopt this standard.
- Curbside infrastructure still has room to grow, especially hydrants, utility cabinets, meters, and lane-edge service objects that would help the streets feel more professionally finished.

## Next cycle goal

Cycle 60 should keep widening the shared realism language at street level:

- target curbside infrastructure such as hydrants, utility cabinets, parking meters, signal-control boxes, or bus-stop sign detail
- keep reducing the places where the city still reads as a clean procedural kit instead of a lived-in Los Angeles street slice
- continue prioritizing reusable procedural helpers that will scale into the eventual many-mile map coverage by default
