# Cycle 58 Report

Date completed: April 18, 2026

## Goal

Push realism outward from the sidewalk and frontage strip into the broader parcel edges that still read as flat graybox fill:

- establish a shared utility-pole and overhead-line language so block perimeters stop ending in visually empty edges
- rebuild the side lots with clearer parking-pad, curb, and planting structure instead of broad undifferentiated dirt rectangles
- vary that shared lot kit enough that residential, retail, and service-spur blocks do not all read with the same outer-parcel weight

## What shipped

- `Sources/EngineCore/engine_core.c` now adds shared utility-pole helpers with taller timber poles, crossarms, transformer hardware, and overhead wire spans so the lot edges can carry a recognizable Los Angeles service silhouette.
- `Sources/EngineCore/engine_core.c` now adds a shared parking-pad helper with asphalt surfacing, curb bands, stall striping, wheel stops, and planter islands so side parcels stop reading as empty lot fill.
- `Sources/EngineCore/engine_core.c` now upgrades `build_block_lot_surfaces` to use those shared helpers across every block, with tag-aware asphalt weight, pad size, planter scale, and extra service aprons instead of one identical side-lot read everywhere.
- `README.md` now advances the repo to Cycle 59, adds manual checks for the new lot-edge realism pass, and updates the prototype summary to call out the utility-line and parking-edge upgrade.

## Notes on implementation

- This stays inside the procedural box language, but it improves the city read by giving the outer block wings a repeatable service and parking vocabulary instead of leaving them as background slabs.
- The utility-line pass is deliberately shared and lightweight: the same pole and wire helpers can scale with the eventual larger road footprint instead of becoming one-off hero props.
- The updated side-lot kit uses block tags to bias asphalt weight and pad size so residential courts stay lighter than service-spur parcels without requiring wholly separate generators.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Walk the outer side lots on several block types and confirm they now read with parking pads, curb edges, striping, wheel stops, and planter islands instead of as plain dirt wings.
- Look along the block perimeter and confirm the utility poles and overhead wires now create a taller service silhouette rather than leaving the lot edges visually blank.
- Compare a residential court block against a service-spur block and confirm the lot kit now shifts in pad size and asphalt weight rather than using one universal parking read.
- Revisit mixed-use and retail blocks and confirm the upgraded back-lot detail supports the stronger storefront pass instead of undercutting it with empty side parcels.

## Blockers and risks

- The poles, wires, and lot edges still rely on layered box composition, so this improves silhouette and structure before the project reaches mesh-level infrastructure fidelity.
- The denser side-lot kit adds more static scene detail across every block, so later full-map scaling work should keep monitoring scene-box count and vertex growth as coverage expands.
- Material variation is still relatively coarse, especially on storefront faces, parking asphalt wear, and utility hardware aging, so another realism pass can still add a lot without changing the core layout.

## Next cycle goal

Cycle 59 should keep pushing realism through shared kits that affect the full map footprint:

- target more varied storefront material treatment, curbside facade color breakup, parking-surface wear, or lane-side infrastructure details
- keep reducing the places where the widened Los Angeles slice still falls back from layered structure to obvious box composition
- continue favoring reusable procedural upgrades that will scale into the eventual many-mile LA road network by default
