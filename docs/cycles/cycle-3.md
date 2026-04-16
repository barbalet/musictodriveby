# Cycle 3 Report

Date completed: April 16, 2026

## Goal

Strengthen the traversal slice by adding:

- mouse look and cleaner camera/input ergonomics
- more landmark variation inside the current connected slice
- stronger curb, corner and obstruction feel tuning
- another pass on third-person follow and placeholder character readability

## What shipped

- Mouse look is now supported in the macOS app flow, with click-to-capture behavior and `Esc` to release it.
- The input path now carries per-frame mouse deltas into the C engine so first-person and third-person camera control both feel more direct.
- Traversal speed now reacts more clearly to sidewalks, curbs, roads and lots, which makes curb transitions and lot cuts feel less flat.
- Collision response now leaves a little more breathing room around solid obstacles and damps blocked movement components instead of letting the player grind against props.
- Ground height now eases across curb transitions instead of snapping immediately, which makes the camera feel steadier at corners.
- Third-person follow now uses movement-aware lead, a slightly smarter chase distance and a more stable focus target.
- The third-person placeholder actor is now easier to read, with a shadowed base, clearer torso/head separation and head tracking that follows the camera direction.
- The graybox slice now has more identity through new landmarks, including a corner store, a fenced half-court, billboard structures and a carport cluster.

## Notes on implementation

- `Sources/MusicToDriveBy` now captures and releases mouse look directly in the Metal view while publishing that state back to the debug overlay.
- `Sources/EngineCore` now accepts analog look deltas, tunes movement speed by surface type and applies softer curb-height interpolation with firmer obstruction damping.
- The world layout is still handcrafted, but the new landmark set gives the current block more recognizable sub-areas for traversal tuning.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Click inside the game view to capture mouse look.
- Press `Esc` to release mouse look if you need the cursor again.
- Toggle to third-person with `C` or `Tab` and walk around the half-court, storefront corner and carport lot to verify landmark readability.
- Open `MusicToDriveBy.xcodeproj` and run the `MusicToDriveBy` scheme on `My Mac` if you want the Xcode app flow.

## Blockers and risks

- Mouse look is now in place, but there is still no full mouse-lock UX beyond capture and release, and controller support is still missing.
- The world has stronger local identity, but it is still one handcrafted slice rather than the beginning of a streamed district.
- Third-person readability is better, but the actor is still a procedural placeholder rather than an animation-backed character.
- Collision and curb feel are improved for graybox play, but there are still no stairs, jumps or richer traversal interactions.

## Next cycle goal

Cycle 4 should keep deepening the on-foot slice by adding:

- denser street furniture and storefront frontage near the main routes
- another pass on first-person readability, including stronger landmark framing and road-crossing visibility
- more nuanced collision silhouettes around props and lot edges
- the first small set of ambient motion hooks so the block starts to feel less static
