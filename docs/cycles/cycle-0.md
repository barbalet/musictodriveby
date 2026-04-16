# Cycle 0 Report

Date completed: April 16, 2026

## Goal

Bootstrap a runnable macOS prototype that proves the project can be split across:

- a SwiftUI application shell
- a Metal render path
- a C engine core

## What shipped

- A macOS Swift package with an executable target for the game client.
- A SwiftUI window that hosts an `MTKView`.
- A Metal renderer that draws a graybox neighborhood block with road, sidewalks and simple building masses.
- A C engine core that owns the first-person camera state, movement update loop and scene definition.
- Keyboard input for movement, turning, looking and sprinting.
- A small in-game debug overlay showing position, camera state, speed and frame rate.
- Build and run instructions in the main README.

## Module boundaries

- `Sources/MusicToDriveBy`: app shell, input capture, renderer, view model and presentation.
- `Sources/EngineCore`: simulation state, update step and static scene box data.
- `docs/cycles`: cycle reports and planning breadcrumbs.

## Coordinate and content conventions

- Right-handed world coordinates.
- `+Y` is up.
- `-Z` is forward when yaw is `0`.
- Distances are treated as meters for movement tuning.
- Graybox geometry is represented as colored boxes so level layout can evolve before asset production begins.

## Build and run checklist

- Build with `swift build`.
- Launch with `swift run MusicToDriveBy`.
- Click the game view if keyboard focus is lost.
- Walk the graybox block in first person using the on-screen controls.

## Blockers and risks

- The bootstrap is macOS-only and currently depends on running from Swift Package Manager rather than an `.app` bundle or Xcode project.
- Shaders are compiled from inline source at runtime, which is fast for iteration but should later move into proper asset handling.
- Collision is currently a simple play-area clamp rather than true curb, sidewalk and building collision.
- The scene is intentionally static and graybox-only; no streaming, content pipeline or authored assets exist yet.

## Next cycle goal

Cycle 1 should turn the bootstrap into a stronger on-foot traversal slice by adding:

- curb and sidewalk collision
- smoother camera handling
- reusable street-segment generation
- a clean first-person / third-person camera toggle
