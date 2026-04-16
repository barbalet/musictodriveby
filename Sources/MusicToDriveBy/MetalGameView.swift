import AppKit
import CoreGraphics
import MetalKit
import SwiftUI

struct MetalGameView: NSViewRepresentable {
    @ObservedObject var viewModel: GameViewModel

    func makeCoordinator() -> Coordinator {
        Coordinator()
    }

    func makeNSView(context: Context) -> GameMTKView {
        guard let device = MTLCreateSystemDefaultDevice() else {
            preconditionFailure("Metal is required to run MusicToDriveBy.")
        }

        let view = GameMTKView(frame: .zero, device: device, inputController: context.coordinator.inputController)
        let renderer = Renderer(
            view: view,
            inputController: context.coordinator.inputController,
            viewModel: viewModel
        )

        context.coordinator.renderer = renderer
        view.delegate = renderer
        DispatchQueue.main.async {
            view.window?.makeFirstResponder(view)
        }
        return view
    }

    func updateNSView(_ nsView: GameMTKView, context: Context) {
        context.coordinator.renderer?.setViewModel(viewModel)
    }

    final class Coordinator {
        let inputController = InputController()
        var renderer: Renderer?
    }
}

final class GameMTKView: MTKView {
    private let inputController: InputController
    private var trackingAreaRef: NSTrackingArea?

    init(frame: CGRect, device: MTLDevice, inputController: InputController) {
        self.inputController = inputController
        super.init(frame: frame, device: device)
        colorPixelFormat = .bgra8Unorm_srgb
        depthStencilPixelFormat = .depth32Float
        clearColor = MTLClearColorMake(0.50, 0.66, 0.84, 1.0)
        preferredFramesPerSecond = 60
        enableSetNeedsDisplay = false
        isPaused = false
    }

    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override var acceptsFirstResponder: Bool {
        true
    }

    override func viewDidMoveToWindow() {
        super.viewDidMoveToWindow()
        window?.acceptsMouseMovedEvents = true
        window?.makeFirstResponder(self)
    }

    override func updateTrackingAreas() {
        super.updateTrackingAreas()

        if let trackingAreaRef {
            removeTrackingArea(trackingAreaRef)
        }

        let trackingArea = NSTrackingArea(
            rect: bounds,
            options: [.activeInKeyWindow, .inVisibleRect, .mouseMoved],
            owner: self,
            userInfo: nil
        )
        addTrackingArea(trackingArea)
        trackingAreaRef = trackingArea
    }

    override func acceptsFirstMouse(for event: NSEvent?) -> Bool {
        true
    }

    override func mouseDown(with event: NSEvent) {
        window?.makeFirstResponder(self)
        setMouseLookEnabled(true)
    }

    override func rightMouseDown(with event: NSEvent) {
        window?.makeFirstResponder(self)
        setMouseLookEnabled(true)
    }

    override func keyDown(with event: NSEvent) {
        if event.keyCode == 53 {
            setMouseLookEnabled(false)
            return
        }

        inputController.handleKeyDown(event.keyCode, isARepeat: event.isARepeat)
    }

    override func keyUp(with event: NSEvent) {
        inputController.handleKeyUp(event.keyCode)
    }

    override func flagsChanged(with event: NSEvent) {
        inputController.handleFlagsChanged(event.modifierFlags)
    }

    override func mouseMoved(with event: NSEvent) {
        inputController.handleMouseDelta(deltaX: event.deltaX, deltaY: event.deltaY)
    }

    override func mouseDragged(with event: NSEvent) {
        inputController.handleMouseDelta(deltaX: event.deltaX, deltaY: event.deltaY)
    }

    override func rightMouseDragged(with event: NSEvent) {
        inputController.handleMouseDelta(deltaX: event.deltaX, deltaY: event.deltaY)
    }

    override func resignFirstResponder() -> Bool {
        setMouseLookEnabled(false)
        inputController.clear()
        return super.resignFirstResponder()
    }

    private func setMouseLookEnabled(_ enabled: Bool) {
        guard enabled != inputController.isMouseLookEnabled else {
            return
        }

        inputController.setMouseLookEnabled(enabled)

        if enabled {
            window?.acceptsMouseMovedEvents = true
            NSCursor.hide()
            _ = CGAssociateMouseAndMouseCursorPosition(0)
        } else {
            _ = CGAssociateMouseAndMouseCursorPosition(1)
            NSCursor.unhide()
        }
    }
}
