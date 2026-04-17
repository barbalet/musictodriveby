import SwiftUI

struct GameRootView: View {
    @ObservedObject var viewModel: GameViewModel
    @ObservedObject var controlPanelController: ControlPanelController
    @State private var showedInitialControls = false

    var body: some View {
        ZStack {
            MetalGameView(viewModel: viewModel)
                .ignoresSafeArea()

            CombatHUDView(model: viewModel.combatHUD)
                .padding(.horizontal, 20)
                .padding(.top, 18)
                .padding(.bottom, 22)
                .allowsHitTesting(false)
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

private struct CombatHUDView: View {
    let model: CombatHUDModel

    private var tint: Color {
        if model.isResetting {
            return Color(red: 0.38, green: 0.88, blue: 0.46)
        }
        if model.isVehicleMode {
            return Color(red: 0.32, green: 0.70, blue: 0.96)
        }
        if model.isCritical {
            return Color(red: 1.0, green: 0.40, blue: 0.28)
        }
        return Color(red: 0.98, green: 0.70, blue: 0.24)
    }

    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Spacer(minLength: 0)
                HUDTopBanner(model: model, tint: tint)
                Spacer(minLength: 0)
            }

            Spacer(minLength: 0)

            VStack(spacing: 14) {
                HUDPromptPill(prompt: model.promptText, tint: tint)

                HStack(alignment: .bottom, spacing: 16) {
                    HUDHealthCard(model: model, tint: tint)

                    Spacer(minLength: 20)

                    VStack(alignment: .trailing, spacing: 12) {
                        HUDDetailCard(
                            section: "Weapon",
                            headline: model.weaponTitle,
                            detail: model.weaponDetail,
                            tint: tint
                        )
                        HUDDetailCard(
                            section: "Encounter",
                            headline: model.encounterTitle,
                            detail: model.encounterDetail,
                            tint: tint
                        )
                        HUDDetailCard(
                            section: "Street",
                            headline: model.systemTitle,
                            detail: model.systemDetail,
                            tint: tint
                        )
                    }
                }
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
}

private struct HUDTopBanner: View {
    let model: CombatHUDModel
    let tint: Color

    var body: some View {
        VStack(alignment: .leading, spacing: 6) {
            Text(model.title)
                .font(.system(size: 17, weight: .bold, design: .rounded))
                .foregroundStyle(.white)

            Text(model.subtitle)
                .font(.system(size: 12, weight: .medium, design: .rounded))
                .foregroundStyle(Color.white.opacity(0.82))
                .fixedSize(horizontal: false, vertical: true)
        }
        .frame(maxWidth: 420, alignment: .leading)
        .padding(.horizontal, 16)
        .padding(.vertical, 12)
        .background(
            LinearGradient(
                colors: [
                    Color.black.opacity(0.78),
                    tint.opacity(0.28),
                ],
                startPoint: .leading,
                endPoint: .trailing
            ),
            in: RoundedRectangle(cornerRadius: 18, style: .continuous)
        )
        .overlay(
            RoundedRectangle(cornerRadius: 18, style: .continuous)
                .stroke(tint.opacity(0.55), lineWidth: 1)
        )
        .shadow(color: .black.opacity(0.35), radius: 18, y: 8)
    }
}

private struct HUDPromptPill: View {
    let prompt: String
    let tint: Color

    var body: some View {
        Text(prompt)
            .font(.system(size: 13, weight: .semibold, design: .rounded))
            .foregroundStyle(.white.opacity(0.94))
            .multilineTextAlignment(.center)
            .lineLimit(3)
            .frame(maxWidth: 760)
            .padding(.horizontal, 18)
            .padding(.vertical, 12)
            .background(
                Color.black.opacity(0.66),
                in: Capsule()
            )
            .overlay(
                Capsule()
                    .stroke(tint.opacity(0.42), lineWidth: 1)
            )
            .shadow(color: .black.opacity(0.28), radius: 12, y: 6)
    }
}

private struct HUDHealthCard: View {
    let model: CombatHUDModel
    let tint: Color

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            Text("Condition")
                .font(.system(size: 11, weight: .semibold, design: .rounded))
                .foregroundStyle(Color.white.opacity(0.68))

            Text(model.healthText)
                .font(.system(size: 24, weight: .bold, design: .rounded))
                .foregroundStyle(.white)

            Text(model.healthDetail)
                .font(.system(size: 12, weight: .medium, design: .rounded))
                .foregroundStyle(Color.white.opacity(0.82))

            ZStack(alignment: .leading) {
                Capsule()
                    .fill(Color.white.opacity(0.12))
                    .frame(width: 188, height: 10)

                Capsule()
                    .fill(
                        LinearGradient(
                            colors: [tint.opacity(0.82), tint],
                            startPoint: .leading,
                            endPoint: .trailing
                        )
                    )
                    .frame(width: max(20, 188 * CGFloat(model.healthRatio)), height: 10)
            }
        }
        .frame(maxWidth: 250, alignment: .leading)
        .padding(.horizontal, 16)
        .padding(.vertical, 14)
        .background(
            Color.black.opacity(0.68),
            in: RoundedRectangle(cornerRadius: 20, style: .continuous)
        )
        .overlay(
            RoundedRectangle(cornerRadius: 20, style: .continuous)
                .stroke(tint.opacity(0.38), lineWidth: 1)
        )
        .shadow(color: .black.opacity(0.30), radius: 14, y: 8)
    }
}

private struct HUDDetailCard: View {
    let section: String
    let headline: String
    let detail: String
    let tint: Color

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text(section)
                .font(.system(size: 11, weight: .semibold, design: .rounded))
                .foregroundStyle(Color.white.opacity(0.68))

            Text(headline)
                .font(.system(size: 15, weight: .bold, design: .rounded))
                .foregroundStyle(.white)

            Text(detail)
                .font(.system(size: 12, weight: .medium, design: .rounded))
                .foregroundStyle(Color.white.opacity(0.82))
                .fixedSize(horizontal: false, vertical: true)
        }
        .frame(width: 240, alignment: .leading)
        .padding(.horizontal, 15)
        .padding(.vertical, 13)
        .background(
            LinearGradient(
                colors: [
                    Color.black.opacity(0.74),
                    tint.opacity(0.14),
                ],
                startPoint: .topLeading,
                endPoint: .bottomTrailing
            ),
            in: RoundedRectangle(cornerRadius: 18, style: .continuous)
        )
        .overlay(
            RoundedRectangle(cornerRadius: 18, style: .continuous)
                .stroke(Color.white.opacity(0.08), lineWidth: 1)
        )
        .shadow(color: .black.opacity(0.28), radius: 12, y: 6)
    }
}
