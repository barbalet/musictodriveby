import Foundation
import simd

@MainActor
final class GameViewModel: ObservableObject {
    @Published var debugSummary = """
    booting renderer...
    """

    let controlsSummary = """
    W A S D: move
    Mouse / trackpad: look when captured
    Q / E or ← / →: turn
    ↑ / ↓: look
    Shift: sprint
    C or Tab: toggle first / third person
    Click in the game view: capture mouse look
    Esc: release mouse look
    Command + /: show or hide controls window
    Green window button or Control + Command + F: fullscreen main window
    Click the game view if keyboard focus is lost
    """

    func update(actorPosition: SIMD3<Float>, cameraPosition: SIMD3<Float>, yaw: Float, pitch: Float, actorHeading: Float, speed: Float, fps: Double, cameraMode: String, surface: String, mouseLook: String, layoutSummary: String, activitySummary: String, currentBlock: String, nearestHook: String) {
        debugSummary = """
        camera: \(cameraMode)
        mouse look: \(mouseLook)
        surface: \(surface)
        layout: \(layoutSummary)
        activity: \(activitySummary)
        block: \(currentBlock)
        nearest hook: \(nearestHook)
        actor: \(Self.format(actorPosition.x)) \(Self.format(actorPosition.y)) \(Self.format(actorPosition.z))
        view: \(Self.format(cameraPosition.x)) \(Self.format(cameraPosition.y)) \(Self.format(cameraPosition.z))
        yaw: \(Self.format(yaw)) rad
        pitch: \(Self.format(pitch)) rad
        heading: \(Self.format(actorHeading)) rad
        speed: \(Self.format(speed)) m/s
        fps: \(String(format: "%.1f", fps))
        """
    }

    private static func format(_ value: Float) -> String {
        String(format: "%.2f", value)
    }
}
