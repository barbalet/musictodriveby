import AppKit
import Foundation

#if SWIFT_PACKAGE
import EngineCore
#endif

final class InputController {
    private var buttons: UInt32 = 0
    private var transientButtons: UInt32 = 0
    private var lookDeltaX: Float = 0
    private var lookDeltaY: Float = 0

    private(set) var isMouseLookEnabled = false

    func handleKeyDown(_ keyCode: UInt16, isARepeat: Bool) {
        if !isARepeat {
            if keyCode == 8 || keyCode == 48 {
                transientButtons |= UInt32(MDTBInputToggleCamera)
                return
            }

            if keyCode == 3 {
                transientButtons |= UInt32(MDTBInputUse)
                return
            }

            if keyCode == 15 {
                transientButtons |= UInt32(MDTBInputCycleHandoff)
                return
            }

            if keyCode == 5 {
                transientButtons |= UInt32(MDTBInputToggleHandoffLock)
                return
            }

            if keyCode == 17 {
                transientButtons |= UInt32(MDTBInputPickupWeapon)
                return
            }

            if keyCode == 18 {
                transientButtons |= UInt32(MDTBInputEquipMeleeWeapon)
                return
            }

            if keyCode == 19 {
                transientButtons |= UInt32(MDTBInputEquipFirearm)
                return
            }

            if keyCode == 16 {
                transientButtons |= UInt32(MDTBInputReloadWeapon)
                return
            }

            if keyCode == 49 {
                transientButtons |= UInt32(MDTBInputAttack)
                return
            }
        }

        setKey(keyCode, isPressed: true)
    }

    func handleKeyUp(_ keyCode: UInt16) {
        setKey(keyCode, isPressed: false)
    }

    func handleFlagsChanged(_ modifiers: NSEvent.ModifierFlags) {
        updateButton(mask: UInt32(MDTBInputSprint), isPressed: modifiers.contains(.shift))
    }

    func handleMouseDelta(deltaX: CGFloat, deltaY: CGFloat) {
        guard isMouseLookEnabled else {
            return
        }

        lookDeltaX += Float(deltaX)
        lookDeltaY += Float(deltaY)
    }

    func setMouseLookEnabled(_ enabled: Bool) {
        isMouseLookEnabled = enabled
        if !enabled {
            lookDeltaX = 0
            lookDeltaY = 0
        }
    }

    func triggerAttack() {
        transientButtons |= UInt32(MDTBInputAttack)
    }

    func clear() {
        buttons = 0
        transientButtons = 0
        lookDeltaX = 0
        lookDeltaY = 0
    }

    func makeFrame(deltaTime: Float) -> MDTBInputFrame {
        let frameButtons = buttons | transientButtons
        let frameLookDeltaX = lookDeltaX
        let frameLookDeltaY = lookDeltaY
        transientButtons = 0
        lookDeltaX = 0
        lookDeltaY = 0
        return MDTBInputFrame(
            delta_time: deltaTime,
            look_delta_x: frameLookDeltaX,
            look_delta_y: frameLookDeltaY,
            buttons: frameButtons
        )
    }

    private func setKey(_ keyCode: UInt16, isPressed: Bool) {
        switch keyCode {
        case 13:
            updateButton(mask: UInt32(MDTBInputMoveForward), isPressed: isPressed)
        case 1:
            updateButton(mask: UInt32(MDTBInputMoveBackward), isPressed: isPressed)
        case 0:
            updateButton(mask: UInt32(MDTBInputMoveLeft), isPressed: isPressed)
        case 2:
            updateButton(mask: UInt32(MDTBInputMoveRight), isPressed: isPressed)
        case 12, 123:
            updateButton(mask: UInt32(MDTBInputTurnLeft), isPressed: isPressed)
        case 14, 124:
            updateButton(mask: UInt32(MDTBInputTurnRight), isPressed: isPressed)
        case 126:
            updateButton(mask: UInt32(MDTBInputLookUp), isPressed: isPressed)
        case 125:
            updateButton(mask: UInt32(MDTBInputLookDown), isPressed: isPressed)
        default:
            break
        }
    }

    private func updateButton(mask: UInt32, isPressed: Bool) {
        if isPressed {
            buttons |= mask
        } else {
            buttons &= ~mask
        }
    }
}
