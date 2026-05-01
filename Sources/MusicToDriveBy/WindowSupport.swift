import AppKit
import SwiftUI

private enum WindowIdentifier {
    static let mainGame = NSUserInterfaceItemIdentifier("by.barbalet.musictodriveby.main-window")
    static let controls = NSUserInterfaceItemIdentifier("by.barbalet.musictodriveby.controls-window")
}

@MainActor
enum MainGameWindowController {
    static func mainWindow() -> NSWindow? {
        NSApp.windows.first { $0.identifier == WindowIdentifier.mainGame }
    }

    static func toggleFullScreen() {
        mainWindow()?.toggleFullScreen(nil)
    }
}

struct MainGameWindowConfigurator: NSViewRepresentable {
    func makeNSView(context: Context) -> NSView {
        let view = NSView(frame: .zero)
        Self.configureWindow(for: view)
        return view
    }

    func updateNSView(_ nsView: NSView, context: Context) {
        Self.configureWindow(for: nsView)
    }

    private static func configureWindow(for view: NSView) {
        DispatchQueue.main.async {
            guard let window = view.window else {
                return
            }

            window.identifier = WindowIdentifier.mainGame
            window.title = "MusicToDriveBy"
            window.tabbingMode = .disallowed
            window.styleMask.insert([.titled, .closable, .miniaturizable, .resizable])
            window.collectionBehavior.insert([.fullScreenPrimary, .managed])
            window.toolbarStyle = .unifiedCompact
        }
    }
}

@MainActor
final class ControlPanelController: NSObject, ObservableObject, NSWindowDelegate {
    @Published private(set) var isVisible = false

    private var panel: NSPanel?

    func show(viewModel: GameViewModel) {
        let panel = makePanelIfNeeded(viewModel: viewModel)
        updateContent(viewModel: viewModel)
        panel.makeKeyAndOrderFront(nil)
        NSApp.activate(ignoringOtherApps: true)
        isVisible = true
    }

    func hide() {
        guard let panel else {
            return
        }

        panel.orderOut(nil)
        MainGameWindowController.mainWindow()?.makeKeyAndOrderFront(nil)
        isVisible = false
    }

    func toggle(viewModel: GameViewModel) {
        if isVisible {
            hide()
        } else {
            show(viewModel: viewModel)
        }
    }

    func windowWillClose(_ notification: Notification) {
        isVisible = false
    }

    func windowDidBecomeKey(_ notification: Notification) {
        isVisible = true
    }

    private func makePanelIfNeeded(viewModel: GameViewModel) -> NSPanel {
        if let panel {
            return panel
        }

        let panel = NSPanel(
            contentRect: NSRect(x: 0, y: 0, width: 380, height: 560),
            styleMask: [.titled, .closable, .resizable, .utilityWindow],
            backing: .buffered,
            defer: false
        )
        panel.title = "Controls"
        panel.identifier = WindowIdentifier.controls
        panel.isFloatingPanel = true
        panel.level = .floating
        panel.hidesOnDeactivate = false
        panel.isReleasedWhenClosed = false
        panel.collectionBehavior = [.fullScreenAuxiliary, .moveToActiveSpace]
        panel.tabbingMode = .disallowed
        panel.toolbarStyle = .unifiedCompact
        panel.standardWindowButton(.miniaturizeButton)?.isHidden = true
        panel.delegate = self
        panel.setFrameAutosaveName("MusicToDriveByControls")
        panel.center()

        self.panel = panel
        updateContent(viewModel: viewModel)
        return panel
    }

    private func updateContent(viewModel: GameViewModel) {
        guard let panel else {
            return
        }

        let rootView = ControlPanelView(viewModel: viewModel, controller: self)
        if let hostingView = panel.contentView as? NSHostingView<ControlPanelView> {
            hostingView.rootView = rootView
        } else {
            panel.contentView = NSHostingView(rootView: rootView)
        }
    }
}

struct MusicToDriveByCommands: Commands {
    @ObservedObject var viewModel: GameViewModel
    @ObservedObject var controlPanelController: ControlPanelController

    var body: some Commands {
        CommandMenu("Game Window") {
            Button(controlPanelController.isVisible ? "Hide Controls" : "Show Controls") {
                controlPanelController.toggle(viewModel: viewModel)
            }
            .keyboardShortcut("/", modifiers: [.command])

            Button("Toggle Full Screen") {
                MainGameWindowController.toggleFullScreen()
            }
            .keyboardShortcut("f", modifiers: [.control, .command])
        }
    }
}
