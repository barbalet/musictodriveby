import SwiftUI

struct GameRootView: View {
    @ObservedObject var viewModel: GameViewModel
    @ObservedObject var controlPanelController: ControlPanelController
    @State private var showedInitialControls = false

    var body: some View {
        ZStack {
            MetalGameView(viewModel: viewModel)
                .ignoresSafeArea()
        }
        .background(Color.black)
        .toolbar {
            ToolbarItemGroup(placement: .automatic) {
                Button {
                    controlPanelController.toggle(viewModel: viewModel)
                } label: {
                    Label(
                        controlPanelController.isVisible ? "Hide Controls" : "Show Controls",
                        systemImage: controlPanelController.isVisible ? "rectangle.compress.vertical" : "switch.2"
                    )
                }
                .help("Show or hide the floating controls panel")

                Button {
                    MainGameWindowController.toggleFullScreen()
                } label: {
                    Label("Full Screen", systemImage: "arrow.up.left.and.arrow.down.right")
                }
                .help("Toggle fullscreen for the main rendering window")
            }
        }
        .onAppear {
            guard !showedInitialControls else {
                return
            }

            showedInitialControls = true
            controlPanelController.show(viewModel: viewModel)
        }
    }
}
