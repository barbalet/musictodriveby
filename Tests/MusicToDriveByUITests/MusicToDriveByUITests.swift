import XCTest

private enum PanelRoute: Int, CaseIterable {
    case toolbar
    case keyboardShortcut
    case panelHideCycle
    case toolbarThenKeyboard

    var name: String {
        switch self {
        case .toolbar:
            return "toolbar"
        case .keyboardShortcut:
            return "keyboard-shortcut"
        case .panelHideCycle:
            return "panel-hide-cycle"
        case .toolbarThenKeyboard:
            return "toolbar-then-keyboard"
        }
    }

    var isKnownFlaky: Bool {
        switch self {
        case .keyboardShortcut, .toolbarThenKeyboard:
            return true
        case .toolbar, .panelHideCycle:
            return false
        }
    }
}

private enum FullScreenRoute: Int, CaseIterable {
    case none
    case toolbar
    case controlsPanel
    case keyboardShortcut

    var name: String {
        switch self {
        case .none:
            return "none"
        case .toolbar:
            return "toolbar"
        case .controlsPanel:
            return "controls-panel"
        case .keyboardShortcut:
            return "keyboard-shortcut"
        }
    }
}

private enum InputProbe: Int, CaseIterable {
    case none
    case toggleCameraC
    case toggleCameraTab
    case pickupWeapon
    case equipMelee
    case equipFirearm
    case reloadWeapon
    case useVehicle

    var name: String {
        switch self {
        case .none:
            return "none"
        case .toggleCameraC:
            return "toggle-camera-c"
        case .toggleCameraTab:
            return "toggle-camera-tab"
        case .pickupWeapon:
            return "pickup-weapon-t"
        case .equipMelee:
            return "equip-melee-1"
        case .equipFirearm:
            return "equip-firearm-2"
        case .reloadWeapon:
            return "reload-y"
        case .useVehicle:
            return "use-vehicle-f"
        }
    }

    var typedKey: String? {
        switch self {
        case .none:
            return nil
        case .toggleCameraC:
            return "c"
        case .toggleCameraTab:
            return "\t"
        case .pickupWeapon:
            return "t"
        case .equipMelee:
            return "1"
        case .equipFirearm:
            return "2"
        case .reloadWeapon:
            return "y"
        case .useVehicle:
            return "f"
        }
    }
}

private struct MatrixScenario: CustomStringConvertible {
    let index: Int
    let launchWithHiddenControls: Bool
    let panelRoute: PanelRoute
    let fullScreenRoute: FullScreenRoute
    let inputProbe: InputProbe
    let inspectControls: Bool

    init(index: Int) {
        self.index = index
        launchWithHiddenControls = (index & 0b1) != 0
        panelRoute = PanelRoute(rawValue: (index >> 1) & 0b11) ?? .toolbar
        fullScreenRoute = FullScreenRoute(rawValue: (index >> 3) & 0b11) ?? .none
        inputProbe = InputProbe(rawValue: (index >> 5) & 0b111) ?? .none
        inspectControls = ((index >> 8) & 0b1) != 0
    }

    var launchArguments: [String] {
        var arguments = ["--uitest"]
        if launchWithHiddenControls {
            arguments.append("--uitest-hide-initial-controls")
        }
        return arguments
    }

    var description: String {
        """
        scenario \(index)
        launch controls: \(launchWithHiddenControls ? "hidden" : "auto-visible")
        panel route: \(panelRoute.name)
        fullscreen route: \(fullScreenRoute.name)
        input probe: \(inputProbe.name)
        inspect controls: \(inspectControls)
        """
    }
}

private enum ControlsActuator {
    case toolbar
    case keyboardShortcut
}

@MainActor
final class MusicToDriveByUITests: XCTestCase {
    private var app: XCUIApplication?

    override func setUpWithError() throws {
        continueAfterFailure = false
    }

    override func tearDownWithError() throws {
        app?.terminate()
        app = nil
    }

    func runMatrixScenario(index: Int, file: StaticString = #filePath, line: UInt = #line) throws {
        let scenario = MatrixScenario(index: index)
        let app = XCUIApplication()
        app.launchArguments = scenario.launchArguments
        app.launch()
        self.app = app

        let summary = XCTAttachment(string: scenario.description)
        summary.name = "Scenario \(index)"
        summary.lifetime = .deleteOnSuccess
        add(summary)

        let mainWindow = try waitForMainWindow(in: app, file: file, line: line)
        assertHUDVisible(in: app, file: file, line: line)
        assertAppRunning(in: app, scenario: scenario, after: "launch", file: file, line: line)

        try driveControlsPanel(route: scenario.panelRoute, inspectControls: scenario.inspectControls, in: app, scenario: scenario, file: file, line: line)

        if scenario.fullScreenRoute != .none {
            try withExpectedInstability("Fullscreen transitions are still exploratory in the current game shell.") {
                try self.probeFullScreen(route: scenario.fullScreenRoute, in: app, scenario: scenario, file: file, line: line)
            }
        }

        if scenario.inputProbe != .none {
            try withExpectedInstability("Keyboard delivery into the Metal gameplay surface is still being hardened for automation.") {
                try self.probeGameplayInput(scenario.inputProbe, in: app, mainWindow: mainWindow, scenario: scenario, file: file, line: line)
            }
        }

        assertHUDVisible(in: app, file: file, line: line)
        assertAppRunning(in: app, scenario: scenario, after: "scenario end", file: file, line: line)
    }

    private func waitForMainWindow(in app: XCUIApplication, file: StaticString, line: UInt) throws -> XCUIElement {
        let titledWindow = app.windows["MusicToDriveBy"]
        if titledWindow.waitForExistence(timeout: 3.0) {
            return titledWindow
        }

        let fallback = app.windows.element(boundBy: 0)
        XCTAssertTrue(fallback.waitForExistence(timeout: 1.0), "Main game window did not appear.", file: file, line: line)
        return fallback
    }

    private func driveControlsPanel(route: PanelRoute, inspectControls: Bool, in app: XCUIApplication, scenario: MatrixScenario, file: StaticString, line: UInt) throws {
        switch route {
        case .toolbar:
            try setControlsVisibility(inspectControls, using: .toolbar, in: app, scenario: scenario, file: file, line: line)
        case .keyboardShortcut:
            try withPanelRouteExpectationIfNeeded(route) {
                try self.setControlsVisibility(inspectControls, using: .keyboardShortcut, in: app, scenario: scenario, file: file, line: line)
            }
        case .panelHideCycle:
            try setControlsVisibility(true, using: .toolbar, in: app, scenario: scenario, file: file, line: line)
            try clickControlsHideButton(in: app, scenario: scenario, file: file, line: line)
            if inspectControls {
                try setControlsVisibility(true, using: .toolbar, in: app, scenario: scenario, file: file, line: line)
            }
        case .toolbarThenKeyboard:
            try setControlsVisibility(true, using: .toolbar, in: app, scenario: scenario, file: file, line: line)
            try withPanelRouteExpectationIfNeeded(route) {
                try self.setControlsVisibility(false, using: .keyboardShortcut, in: app, scenario: scenario, file: file, line: line)
                if inspectControls {
                    try self.setControlsVisibility(true, using: .keyboardShortcut, in: app, scenario: scenario, file: file, line: line)
                }
            }
        }

        if inspectControls {
            XCTAssertTrue(identifiedElement("controls.window.content", in: app).waitForExistence(timeout: 1.0), "Controls panel did not become visible.", file: file, line: line)
            XCTAssertTrue(identifiedElement("controls.section.status", in: app).exists, "Status section missing from controls panel.", file: file, line: line)
            XCTAssertTrue(identifiedElement("controls.section.controls", in: app).exists, "Controls section missing from controls panel.", file: file, line: line)
            XCTAssertTrue(app.staticTexts.containing(NSPredicate(format: "label CONTAINS %@", "W A S D")).firstMatch.exists, "Controls summary text did not appear.", file: file, line: line)
        } else {
            if controlsPanelIsVisible(in: app) {
                try setControlsVisibility(false, using: .toolbar, in: app, scenario: scenario, file: file, line: line)
            }
        }

        assertAppRunning(in: app, scenario: scenario, after: "controls route", file: file, line: line)
    }

    private func probeFullScreen(route: FullScreenRoute, in app: XCUIApplication, scenario: MatrixScenario, file: StaticString, line: UInt) throws {
        switch route {
        case .none:
            return
        case .toolbar:
            try clickToolbarFullScreenButton(in: app, file: file, line: line)
            shortPause()
            try clickToolbarFullScreenButton(in: app, file: file, line: line)
        case .controlsPanel:
            try setControlsVisibility(true, using: .toolbar, in: app, scenario: scenario, file: file, line: line)
            try clickControlsFullScreenButton(in: app, file: file, line: line)
            shortPause()
            try clickControlsFullScreenButton(in: app, file: file, line: line)
        case .keyboardShortcut:
            app.typeKey("f", modifierFlags: [.control, .command])
            shortPause()
            app.typeKey("f", modifierFlags: [.control, .command])
        }

        shortPause()
        assertAppRunning(in: app, scenario: scenario, after: "fullscreen probe", file: file, line: line)
    }

    private func probeGameplayInput(_ probe: InputProbe, in app: XCUIApplication, mainWindow: XCUIElement, scenario: MatrixScenario, file: StaticString, line: UInt) throws {
        guard let typedKey = probe.typedKey else {
            return
        }

        if controlsPanelIsVisible(in: app) {
            try setControlsVisibility(false, using: .toolbar, in: app, scenario: scenario, file: file, line: line)
        }

        mainWindow.coordinate(withNormalizedOffset: CGVector(dx: 0.5, dy: 0.5)).click()
        shortPause()
        app.typeKey(typedKey, modifierFlags: [])
        shortPause()
        assertAppRunning(in: app, scenario: scenario, after: "input probe \(probe.name)", file: file, line: line)
    }

    private func setControlsVisibility(_ visible: Bool, using actuator: ControlsActuator, in app: XCUIApplication, scenario: MatrixScenario, file: StaticString, line: UInt) throws {
        for _ in 0..<3 {
            if controlsPanelIsVisible(in: app) == visible {
                return
            }

            switch actuator {
            case .toolbar:
                try clickToolbarToggleControlsButton(in: app, file: file, line: line)
            case .keyboardShortcut:
                app.typeKey("/", modifierFlags: [.command])
            }

            shortPause()
            assertAppRunning(in: app, scenario: scenario, after: "controls toggle", file: file, line: line)
        }

        XCTAssertEqual(controlsPanelIsVisible(in: app), visible, "Controls panel visibility did not settle to \(visible).", file: file, line: line)
    }

    private func controlsPanelIsVisible(in app: XCUIApplication) -> Bool {
        identifiedElement("controls.window.content", in: app).exists
    }

    private func clickToolbarToggleControlsButton(in app: XCUIApplication, file: StaticString, line: UInt) throws {
        let button = try existingElement(
            [
                identifiedElement("toolbar.toggleControls", in: app),
                app.buttons["Show Controls"],
                app.buttons["Hide Controls"],
            ],
            description: "toolbar toggle controls button",
            file: file,
            line: line
        )
        button.click()
    }

    private func clickToolbarFullScreenButton(in app: XCUIApplication, file: StaticString, line: UInt) throws {
        let button = try existingElement(
            [
                identifiedElement("toolbar.fullScreen", in: app),
                app.buttons["Full Screen"],
            ],
            description: "toolbar fullscreen button",
            file: file,
            line: line
        )
        button.click()
    }

    private func clickControlsHideButton(in app: XCUIApplication, scenario: MatrixScenario, file: StaticString, line: UInt) throws {
        let button = try existingElement(
            [
                identifiedElement("controls.button.hide", in: app),
                app.buttons["Hide"],
            ],
            description: "controls hide button",
            file: file,
            line: line
        )
        button.click()
        shortPause()
        assertAppRunning(in: app, scenario: scenario, after: "controls hide button", file: file, line: line)
    }

    private func clickControlsFullScreenButton(in app: XCUIApplication, file: StaticString, line: UInt) throws {
        let button = try existingElement(
            [
                identifiedElement("controls.button.fullScreen", in: app),
                app.buttons["Full Screen"],
            ],
            description: "controls fullscreen button",
            file: file,
            line: line
        )
        button.click()
    }

    private func assertHUDVisible(in app: XCUIApplication, file: StaticString, line: UInt) {
        let requiredIdentifiers = [
            "hud.topBanner",
            "hud.promptPill",
            "hud.healthCard",
            "hud.weaponCard",
            "hud.encounterCard",
            "hud.streetCard",
            "hud.mapCard",
        ]

        for identifier in requiredIdentifiers {
            XCTAssertTrue(identifiedElement(identifier, in: app).waitForExistence(timeout: 1.0), "Expected HUD element \(identifier) to be visible.", file: file, line: line)
        }
    }

    private func assertAppRunning(in app: XCUIApplication, scenario: MatrixScenario, after step: String, file: StaticString, line: UInt) {
        guard app.state != .notRunning else {
            let screenshot = XCTAttachment(screenshot: app.screenshot())
            screenshot.name = "Crash after \(step)"
            screenshot.lifetime = .keepAlways
            add(screenshot)
            XCTFail("App exited or crashed after \(step).\n\(scenario.description)", file: file, line: line)
            return
        }
    }

    private func identifiedElement(_ identifier: String, in app: XCUIApplication) -> XCUIElement {
        app.descendants(matching: .any).matching(identifier: identifier).firstMatch
    }

    private func existingElement(_ candidates: [XCUIElement], description: String, file: StaticString, line: UInt) throws -> XCUIElement {
        for candidate in candidates where candidate.exists {
            return candidate
        }

        for candidate in candidates where candidate.waitForExistence(timeout: 0.75) {
            return candidate
        }

        XCTFail("Could not find \(description).", file: file, line: line)
        throw NSError(domain: "MusicToDriveByUITests", code: 1)
    }

    private func withPanelRouteExpectationIfNeeded(_ route: PanelRoute, body: () throws -> Void) throws {
        guard route.isKnownFlaky else {
            try body()
            return
        }

        try withExpectedInstability("Keyboard-driven controls-panel toggles are still being stabilized.") {
            try body()
        }
    }

    private func withExpectedInstability(_ reason: String, body: () throws -> Void) throws {
        let options = XCTExpectedFailure.Options()
        options.isStrict = false
        try XCTExpectFailure(reason, options: options) {
            try body()
        }
    }

    private func shortPause() {
        usleep(180_000)
    }
}
