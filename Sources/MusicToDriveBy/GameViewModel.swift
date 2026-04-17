import Foundation
import simd

struct CombatHUDModel: Equatable {
    var title = "Combat Lane"
    var subtitle = "Approach the sandbox"
    var healthRatio: Double = 1.0
    var healthText = "100 hp"
    var healthDetail = "Lane calm"
    var weaponTitle = "Unarmed"
    var weaponDetail = "Pick up a weapon"
    var encounterTitle = "Lookout idle"
    var encounterDetail = "No pressure yet"
    var systemTitle = "Street calm"
    var systemDetail = "No witness reaction yet"
    var promptText = "Walk north to the combat lane"
    var isCritical = false
    var isVehicleMode = false
    var isResetting = false
}

@MainActor
final class GameViewModel: ObservableObject {
    @Published var debugSummary = """
    booting renderer...
    """
    @Published var combatHUD = CombatHUDModel()

    let controlsSummary = """
    W A S D: walk, or throttle and steer while driving
    Mouse / trackpad: look when captured
    Space or left click while captured: attack or fire the equipped weapon
    Q / E or ← / →: turn
    ↑ / ↓: look
    Shift: sprint
    F: enter or exit the selected staged vehicle when prompted
    T: grab the nearest weapon pickup when you are close to it
    1 / 2: equip lead pipe or pistol when owned
    Y: reload the pistol
    R: cycle staged vehicle candidates
    G: lock or unlock the current handoff candidate
    C or Tab: toggle first / third person
    Click in the game view: capture mouse look
    Esc: release mouse look
    Command + /: show or hide controls window
    Green window button or Control + Command + F: fullscreen main window
    Click the game view if keyboard focus is lost
    """

    func update(actorPosition: SIMD3<Float>, cameraPosition: SIMD3<Float>, yaw: Float, pitch: Float, actorHeading: Float, speed: Float, fps: Double, cameraMode: String, surface: String, mouseLook: String, layoutSummary: String, activitySummary: String, vehicleStatus: String, combatSummary: String, interactionSummary: String, selectionSummary: String, hazardSummary: String, currentBlock: String, nearestHook: String, combatHUD: CombatHUDModel) {
        debugSummary = """
        camera: \(cameraMode)
        mouse look: \(mouseLook)
        surface: \(surface)
        layout: \(layoutSummary)
        activity: \(activitySummary)
        vehicle: \(vehicleStatus)
        combat: \(combatSummary)
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
        self.combatHUD = combatHUD
    }

    private static func format(_ value: Float) -> String {
        String(format: "%.2f", value)
    }
}
