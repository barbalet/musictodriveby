import Foundation

enum UITestSupport {
    private static let arguments = ProcessInfo.processInfo.arguments

    static var isEnabled: Bool {
        arguments.contains("--uitest")
    }

    static var hideInitialControls: Bool {
        arguments.contains("--uitest-hide-initial-controls")
    }
}
