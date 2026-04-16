# Cycle 4 Report

Date completed: April 16, 2026

## Goal

Deepen the on-foot slice by adding:

- denser street furniture and storefront frontage near the main routes
- another pass on first-person readability, including stronger landmark framing and road-crossing visibility
- more nuanced collision silhouettes around props and lot edges
- the first small set of ambient motion hooks so the block starts to feel less static

## What shipped

- The main sidewalks now have denser frontage dressing, including awnings, newsstands, bins, benches, crates and small shelter structures near the routes the player naturally follows.
- The corner store and the shop-facing buildings near the central roads now read more clearly as destinations instead of generic block volumes.
- First-person readability is stronger through added lane arrows, curbside bollards and denser route framing around the central crossings.
- Several coarse collision shapes were replaced or supplemented with narrower segmented silhouettes, especially around benches, frontage props and lot-edge fencing.
- Lot boundaries now use more articulated fence runs instead of relying only on broad blocking shapes, which creates cleaner edge definition during traversal.
- The renderer now adds a lightweight ambient layer with animated signal lamps, a swinging storefront sign, and small animated accent props so the slice feels less frozen.
- First-person view now uses a slightly wider field of view than third-person, which helps the current road-crossing and landmark pass read more cleanly.

## Notes on implementation

- `Sources/EngineCore` now builds more detailed route-side frontage and segmented lot-edge collision runs from reusable helper functions.
- Road markings were extended with directional arrows so the main intersection has clearer approach language when viewed on foot.
- `Sources/MusicToDriveBy` now appends a small dynamic-vertex layer on top of the static graybox scene rather than rebuilding the entire world each frame.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Click inside the game view to capture mouse look and press `Esc` to release it.
- Walk the main intersection in first-person and confirm the lane arrows, bollards and storefront frontage make crossing direction easier to read.
- Toggle to third-person with `C` or `Tab` and check that the ambient signal lamps and storefront sign motion remain readable without harming camera follow.
- Open `MusicToDriveBy.xcodeproj` and run the `MusicToDriveBy` scheme on `My Mac` if you want the Xcode app flow.

## Blockers and risks

- The ambient layer is still presentation-only; there are no simulated pedestrians, vehicles or systemic world actors yet.
- The block now reads more clearly, but the world is still one handcrafted slice rather than a reusable multi-block layout.
- Collision silhouettes are more nuanced around select props and lot edges, but the broader world still relies on simple box-based graybox geometry.
- Route readability is better, but there is still no minimap, mission guidance or interaction language beyond environmental cues.

## Next cycle goal

Cycle 5 should begin the world-layout phase by adding:

- the first reusable block or chunk descriptors for assembling more than one authored slice
- a second connected block extension beyond the current central intersection
- clearer boundaries between scene generation data and the renderer-facing box export
- the first spawn or interest-point hooks that future streaming and population systems can attach to
