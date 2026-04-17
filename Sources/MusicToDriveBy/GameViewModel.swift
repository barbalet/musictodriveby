import Foundation
import simd

@MainActor
final class GameViewModel: ObservableObject {
    @Published var debugSummary = """
    booting renderer...
    """

    let controlsSummary = """
    W A S D: walk, or throttle and steer while driving
    Mouse / trackpad: look when captured
    Q / E or ← / →: turn
    ↑ / ↓: look
    Shift: sprint
    F: enter or exit the selected staged vehicle when prompted
    R: cycle staged vehicle candidates
    G: lock or unlock the current handoff candidate
    C or Tab: toggle first / third person
    Click in the game view: capture mouse look
    Esc: release mouse look
    Command + /: show or hide controls window
    Green window button or Control + Command + F: fullscreen main window
    Click the game view if keyboard focus is lost
    """

    func update(actorPosition: SIMD3<Float>, cameraPosition: SIMD3<Float>, yaw: Float, pitch: Float, actorHeading: Float, speed: Float, fps: Double, cameraMode: String, surface: String, mouseLook: String, layoutSummary: String, activitySummary: String, vehicleStatus: String, interactionSummary: String, selectionSummary: String, hazardSummary: String, currentBlock: String, nearestHook: String) {
        debugSummary = """
        camera: \(cameraMode)
        mouse look: \(mouseLook)
        surface: \(surface)
        layout: \(layoutSummary)
        activity: \(activitySummary)
        vehicle: \(vehicleStatus)
        prompt: \(interactionSummary)
        handoff: \(selectionSummary)
        hazard: \(hazardSummary)
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
