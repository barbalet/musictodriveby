import SwiftUI

struct GameRootView: View {
    @StateObject private var viewModel = GameViewModel()

    var body: some View {
        ZStack(alignment: .topLeading) {
            MetalGameView(viewModel: viewModel)
                .ignoresSafeArea()

            VStack(alignment: .leading, spacing: 12) {
                Text("MusicToDriveBy")
                    .font(.system(size: 28, weight: .bold, design: .rounded))

                Text("Cycle 4 streetscape and ambient pass")
                    .font(.system(size: 15, weight: .semibold, design: .rounded))
                    .foregroundStyle(.secondary)

                Divider()

                Text(viewModel.debugSummary)
                    .font(.system(size: 13, weight: .regular, design: .monospaced))
                    .textSelection(.enabled)

                Divider()

                Text(viewModel.controlsSummary)
                    .font(.system(size: 13, weight: .regular, design: .monospaced))
                    .foregroundStyle(.secondary)
            }
            .padding(18)
            .background(.ultraThinMaterial, in: RoundedRectangle(cornerRadius: 18, style: .continuous))
            .padding(24)
        }
        .background(Color.black)
    }
}
