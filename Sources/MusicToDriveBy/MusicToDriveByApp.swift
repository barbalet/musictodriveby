import SwiftUI

@main
struct MusicToDriveByApp: App {
    @StateObject private var viewModel = GameViewModel()
    @StateObject private var controlPanelController = ControlPanelController()

    var body: some Scene {
        WindowGroup {
            GameRootView(viewModel: viewModel, controlPanelController: controlPanelController)
                .frame(minWidth: 1280, minHeight: 800)
                .background(MainGameWindowConfigurator())
        }
        .commands {
            MusicToDriveByCommands(viewModel: viewModel, controlPanelController: controlPanelController)
        }
    }
}
