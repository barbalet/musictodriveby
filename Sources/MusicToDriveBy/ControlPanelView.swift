import SwiftUI

struct ControlPanelView: View {
    @ObservedObject var viewModel: GameViewModel
    @ObservedObject var controller: ControlPanelController

    var body: some View {
        VStack(spacing: 0) {
            HStack(alignment: .top) {
                VStack(alignment: .leading, spacing: 4) {
                    Text("MusicToDriveBy")
                        .font(.system(size: 20, weight: .bold, design: .rounded))

                    Text("Controls and debug window")
                        .font(.system(size: 13, weight: .semibold, design: .rounded))
                        .foregroundStyle(.secondary)
                }

                Spacer()

                Button("Full Screen") {
                    MainGameWindowController.toggleFullScreen()
                }
                .accessibilityIdentifier("controls.button.fullScreen")

                Button("Hide") {
                    controller.hide()
                }
                .accessibilityIdentifier("controls.button.hide")
            }
            .padding(18)

            Divider()

            ScrollView {
                VStack(alignment: .leading, spacing: 18) {
                    PanelSection(title: "Status", bodyText: viewModel.debugSummary, usesSecondaryStyle: false, accessibilityID: "controls.section.status")
                    PanelSection(title: "Controls", bodyText: viewModel.controlsSummary, usesSecondaryStyle: true, accessibilityID: "controls.section.controls")
                }
                .padding(18)
            }
        }
        .frame(minWidth: 360, minHeight: 520)
        .background(Color(nsColor: .windowBackgroundColor))
        .accessibilityIdentifier("controls.window.content")
    }
}

private struct PanelSection: View {
    let title: String
    let bodyText: String
    let usesSecondaryStyle: Bool
    let accessibilityID: String

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            Text(title)
                .font(.system(size: 13, weight: .semibold, design: .rounded))
                .foregroundStyle(.secondary)

            Text(bodyText)
                .font(.system(size: 13, weight: .regular, design: .monospaced))
                .foregroundStyle(usesSecondaryStyle ? .secondary : .primary)
                .textSelection(.enabled)
                .frame(maxWidth: .infinity, alignment: .leading)
                .padding(14)
                .background(.quaternary.opacity(0.35), in: RoundedRectangle(cornerRadius: 14, style: .continuous))
        }
        .accessibilityElement(children: .contain)
        .accessibilityIdentifier(accessibilityID)
    }
}
