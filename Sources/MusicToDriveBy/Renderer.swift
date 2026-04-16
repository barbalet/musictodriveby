import Metal
import MetalKit
import QuartzCore
import Foundation
import simd

#if SWIFT_PACKAGE
import EngineCore
#endif

private struct Vertex {
    var position: SIMD4<Float>
    var color: SIMD4<Float>
}

private struct Uniforms {
    var viewProjectionMatrix: simd_float4x4
}

@MainActor
final class Renderer: NSObject, MTKViewDelegate {
    private static let dynamicVertexBudget = 28 * 36

    private let device: MTLDevice
    private let commandQueue: MTLCommandQueue
    private let pipelineState: MTLRenderPipelineState
    private let depthState: MTLDepthStencilState
    private let vertexBuffer: MTLBuffer
    private let sceneVertexCount: Int
    private let maxVertexCount: Int
    private let inputController: InputController

    private weak var viewModel: GameViewModel?
    private var engineState = MDTBEngineState()
    private var lastFrameTime = CACurrentMediaTime()
    private var fpsWindowTime = 0.0
    private var fpsWindowFrames = 0
    private var latestFPS = 60.0

    init(view: MTKView, inputController: InputController, viewModel: GameViewModel) {
        guard let commandQueue = view.device?.makeCommandQueue() else {
            preconditionFailure("Unable to create Metal command queue.")
        }

        self.device = view.device!
        self.commandQueue = commandQueue
        self.inputController = inputController
        self.viewModel = viewModel
        mdtb_engine_init(&engineState)

        let vertexDescriptor = Renderer.makeVertexDescriptor()
        self.pipelineState = Renderer.makePipelineState(device: self.device, pixelFormat: view.colorPixelFormat, depthFormat: view.depthStencilPixelFormat, vertexDescriptor: vertexDescriptor)
        self.depthState = Renderer.makeDepthState(device: self.device)

        let sceneVertices = Renderer.buildSceneVertices()
        self.sceneVertexCount = sceneVertices.count
        self.maxVertexCount = sceneVertices.count + Self.dynamicVertexBudget

        guard let vertexBuffer = self.device.makeBuffer(length: MemoryLayout<Vertex>.stride * maxVertexCount, options: .storageModeShared) else {
            preconditionFailure("Unable to allocate vertex buffer.")
        }

        _ = sceneVertices.withUnsafeBytes { bytes in
            memcpy(vertexBuffer.contents(), bytes.baseAddress, bytes.count)
        }

        vertexBuffer.label = "Graybox Scene"
        self.vertexBuffer = vertexBuffer
        super.init()
    }

    func setViewModel(_ viewModel: GameViewModel) {
        self.viewModel = viewModel
    }

    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
    }

    func draw(in view: MTKView) {
        guard
            let renderPassDescriptor = view.currentRenderPassDescriptor,
            let drawable = view.currentDrawable
        else {
            return
        }

        let now = CACurrentMediaTime()
        let deltaTime = Float(min(max(now - lastFrameTime, 1.0 / 240.0), 1.0 / 20.0))
        lastFrameTime = now

        let frame = inputController.makeFrame(deltaTime: deltaTime)
        mdtb_engine_step(&engineState, frame)

        fpsWindowTime += Double(deltaTime)
        fpsWindowFrames += 1
        if fpsWindowTime >= 0.25 {
            latestFPS = Double(fpsWindowFrames) / fpsWindowTime
            fpsWindowTime = 0
            fpsWindowFrames = 0
            publishDebugState()
        }

        let cameraPosition = SIMD3<Float>(
            engineState.camera.position.x,
            engineState.camera.position.y,
            engineState.camera.position.z
        )

        let focusPosition = SIMD3<Float>(
            engineState.camera.focus_position.x,
            engineState.camera.focus_position.y,
            engineState.camera.focus_position.z
        )

        let cameraForward = SIMD3<Float>(
            sin(engineState.camera.yaw) * cos(engineState.camera.pitch),
            sin(engineState.camera.pitch),
            -cos(engineState.camera.yaw) * cos(engineState.camera.pitch)
        )

        let isThirdPerson = engineState.camera.mode == UInt32(MDTBCameraModeThirdPerson)
        let ambientVertices = Self.makeAmbientVertices(elapsedTime: engineState.elapsed_time)
        let actorVertices = isThirdPerson
            ? Self.makeActorVertices(
                position: SIMD3<Float>(engineState.actor_position.x, engineState.actor_position.y, engineState.actor_position.z),
                heading: engineState.actor_heading,
                cameraYaw: engineState.camera.yaw,
                speed: engineState.camera.move_speed,
                elapsedTime: engineState.elapsed_time
            )
            : []
        let dynamicVertices = ambientVertices + actorVertices
        let drawVertexCount = sceneVertexCount + dynamicVertices.count

        if !dynamicVertices.isEmpty {
            _ = dynamicVertices.withUnsafeBytes { bytes in
                memcpy(
                    vertexBuffer.contents().advanced(by: MemoryLayout<Vertex>.stride * sceneVertexCount),
                    bytes.baseAddress,
                    bytes.count
                )
            }
        }

        let drawableWidth = max(Float(view.drawableSize.width), 1)
        let drawableHeight = max(Float(view.drawableSize.height), 1)
        let aspect = drawableWidth / drawableHeight

        let projection = simd_float4x4.perspectiveProjection(
            fovY: (isThirdPerson ? 70 : 74) * (.pi / 180),
            aspect: aspect,
            nearZ: 0.1,
            farZ: 200
        )

        let viewMatrix = simd_float4x4.lookAt(
            eye: cameraPosition,
            target: isThirdPerson ? focusPosition : cameraPosition + cameraForward,
            up: SIMD3<Float>(0, 1, 0)
        )

        var uniforms = Uniforms(viewProjectionMatrix: projection * viewMatrix)

        guard
            let commandBuffer = commandQueue.makeCommandBuffer(),
            let renderEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor)
        else {
            return
        }

        commandBuffer.label = "MusicToDriveBy Frame"
        renderEncoder.label = "Graybox Encoder"
        renderEncoder.pushDebugGroup("Graybox Block")
        renderEncoder.setRenderPipelineState(pipelineState)
        renderEncoder.setDepthStencilState(depthState)
        renderEncoder.setCullMode(.back)
        renderEncoder.setFrontFacing(.counterClockwise)
        renderEncoder.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
        renderEncoder.setVertexBytes(&uniforms, length: MemoryLayout<Uniforms>.stride, index: 1)
        renderEncoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: drawVertexCount)
        renderEncoder.popDebugGroup()
        renderEncoder.endEncoding()

        commandBuffer.present(drawable)
        commandBuffer.commit()
    }

    private func publishDebugState() {
        let actorPosition = SIMD3<Float>(
            engineState.actor_position.x,
            engineState.actor_position.y,
            engineState.actor_position.z
        )

        let cameraPosition = SIMD3<Float>(
            engineState.camera.position.x,
            engineState.camera.position.y,
            engineState.camera.position.z
        )

        let snapshotYaw = engineState.camera.yaw
        let snapshotPitch = engineState.camera.pitch
        let snapshotHeading = engineState.actor_heading
        let snapshotSpeed = engineState.camera.move_speed
        let snapshotFPS = latestFPS
        let snapshotCameraMode = Self.cameraModeLabel(engineState.camera.mode)
        let snapshotSurface = Self.surfaceLabel(engineState.surface_kind)

        viewModel?.update(
            actorPosition: actorPosition,
            cameraPosition: cameraPosition,
            yaw: snapshotYaw,
            pitch: snapshotPitch,
            actorHeading: snapshotHeading,
            speed: snapshotSpeed,
            fps: snapshotFPS,
            cameraMode: snapshotCameraMode,
            surface: snapshotSurface,
            mouseLook: inputController.isMouseLookEnabled ? "captured" : "free"
        )
    }

    private static func makeVertexDescriptor() -> MTLVertexDescriptor {
        let descriptor = MTLVertexDescriptor()
        descriptor.attributes[0].format = .float4
        descriptor.attributes[0].offset = 0
        descriptor.attributes[0].bufferIndex = 0
        descriptor.attributes[1].format = .float4
        descriptor.attributes[1].offset = MemoryLayout<SIMD4<Float>>.stride
        descriptor.attributes[1].bufferIndex = 0
        descriptor.layouts[0].stride = MemoryLayout<Vertex>.stride
        return descriptor
    }

    private static func makePipelineState(device: MTLDevice, pixelFormat: MTLPixelFormat, depthFormat: MTLPixelFormat, vertexDescriptor: MTLVertexDescriptor) -> MTLRenderPipelineState {
        let library: MTLLibrary
        do {
            library = try device.makeLibrary(source: ShaderSource.grayboxScene, options: nil)
        } catch {
            preconditionFailure("Unable to compile Metal shader source: \(error)")
        }

        let descriptor = MTLRenderPipelineDescriptor()
        descriptor.label = "Graybox Pipeline"
        descriptor.vertexDescriptor = vertexDescriptor
        descriptor.vertexFunction = library.makeFunction(name: "vertex_main")
        descriptor.fragmentFunction = library.makeFunction(name: "fragment_main")
        descriptor.colorAttachments[0].pixelFormat = pixelFormat
        descriptor.depthAttachmentPixelFormat = depthFormat

        do {
            return try device.makeRenderPipelineState(descriptor: descriptor)
        } catch {
            preconditionFailure("Unable to create render pipeline: \(error)")
        }
    }

    private static func makeDepthState(device: MTLDevice) -> MTLDepthStencilState {
        let descriptor = MTLDepthStencilDescriptor()
        descriptor.depthCompareFunction = .less
        descriptor.isDepthWriteEnabled = true
        guard let state = device.makeDepthStencilState(descriptor: descriptor) else {
            preconditionFailure("Unable to create depth stencil state.")
        }
        return state
    }

    private static func buildSceneVertices() -> [Vertex] {
        let boxCount = Int(mdtb_engine_box_count())
        let boxes = UnsafeMutablePointer<MDTBBox>.allocate(capacity: boxCount)
        defer {
            boxes.deallocate()
        }

        mdtb_engine_copy_boxes(boxes, boxCount)

        var vertices: [Vertex] = []
        vertices.reserveCapacity(boxCount * 36)

        for index in 0 ..< boxCount {
            vertices.append(contentsOf: makeBoxVertices(boxes[index]))
        }

        return vertices
    }

    private static func makeAmbientVertices(elapsedTime: Float) -> [Vertex] {
        let signalPhase = elapsedTime.truncatingRemainder(dividingBy: 8.0)
        let signalColor: SIMD4<Float>

        switch signalPhase {
        case 0 ..< 3.0:
            signalColor = SIMD4<Float>(0.22, 0.88, 0.31, 1.0)
        case 3.0 ..< 4.2:
            signalColor = SIMD4<Float>(0.93, 0.74, 0.16, 1.0)
        default:
            signalColor = SIMD4<Float>(0.88, 0.22, 0.18, 1.0)
        }

        let signYaw = sin(elapsedTime * 1.35) * 0.24
        let pennantBase = elapsedTime * 2.8

        return
            makeWorldBoxVertices(center: SIMD3<Float>(-8.9, 2.42, -9.35), halfExtents: SIMD3<Float>(0.14, 0.14, 0.14), yaw: 0.0, color: signalColor) +
            makeWorldBoxVertices(center: SIMD3<Float>(8.9, 2.42, -9.35), halfExtents: SIMD3<Float>(0.14, 0.14, 0.14), yaw: 0.0, color: signalColor) +
            makeWorldBoxVertices(center: SIMD3<Float>(-8.9, 2.42, 9.35), halfExtents: SIMD3<Float>(0.14, 0.14, 0.14), yaw: 0.0, color: signalColor) +
            makeWorldBoxVertices(center: SIMD3<Float>(8.9, 2.42, 9.35), halfExtents: SIMD3<Float>(0.14, 0.14, 0.14), yaw: 0.0, color: signalColor) +
            makeWorldBoxVertices(center: SIMD3<Float>(-37.15, 2.06, -13.35), halfExtents: SIMD3<Float>(0.84, 0.42, 0.08), yaw: signYaw, color: SIMD4<Float>(0.90, 0.74, 0.22, 1.0)) +
            makeWorldBoxVertices(center: SIMD3<Float>(-44.9, 2.62 + (sin(pennantBase + 0.2) * 0.05), -11.8), halfExtents: SIMD3<Float>(0.28, 0.03, 0.12), yaw: 0.0, color: SIMD4<Float>(0.86, 0.36, 0.19, 1.0)) +
            makeWorldBoxVertices(center: SIMD3<Float>(-42.8, 2.60 + (sin(pennantBase + 0.8) * 0.05), -11.85), halfExtents: SIMD3<Float>(0.28, 0.03, 0.12), yaw: 0.0, color: SIMD4<Float>(0.90, 0.80, 0.28, 1.0)) +
            makeWorldBoxVertices(center: SIMD3<Float>(-40.7, 2.63 + (sin(pennantBase + 1.4) * 0.05), -11.8), halfExtents: SIMD3<Float>(0.28, 0.03, 0.12), yaw: 0.0, color: SIMD4<Float>(0.27, 0.54, 0.71, 1.0)) +
            makeWorldBoxVertices(center: SIMD3<Float>(10.1, 2.18 + (sin((elapsedTime * 1.8) + 1.1) * 0.06), 34.0), halfExtents: SIMD3<Float>(0.58, 0.18, 0.06), yaw: 0.0, color: SIMD4<Float>(0.88, 0.91, 0.95, 1.0))
    }

    private static func makeActorVertices(position: SIMD3<Float>, heading: Float, cameraYaw: Float, speed: Float, elapsedTime: Float) -> [Vertex] {
        let gaitFrequency: Float = 6.0 + (speed * 0.45)
        let gaitPhase = elapsedTime * gaitFrequency
        let strideIntensity = min(speed / 6.0, 1.0)
        let stride = sin(gaitPhase) * strideIntensity * 0.14
        let armSwing = sin(gaitPhase + (Float.pi * 0.5)) * strideIntensity * 0.12
        let headDelta = max(min(Self.wrapAngle(cameraYaw - heading), 0.75), -0.75)
        let torsoYaw = heading + (headDelta * 0.22)
        let headYaw = heading + (headDelta * 0.56)

        return
            makeActorPartVertices(position: position, localCenter: SIMD3<Float>(0.0, 0.04, 0.02), halfExtents: SIMD3<Float>(0.44, 0.02, 0.72), bodyYaw: heading, partYaw: heading, color: SIMD4<Float>(0.10, 0.11, 0.13, 1.0)) +
            makeActorPartVertices(position: position, localCenter: SIMD3<Float>(0.0, 1.02, 0.0), halfExtents: SIMD3<Float>(0.34, 0.50, 0.18), bodyYaw: heading, partYaw: torsoYaw, color: SIMD4<Float>(0.91, 0.39, 0.22, 1.0)) +
            makeActorPartVertices(position: position, localCenter: SIMD3<Float>(0.0, 1.14, -0.22), halfExtents: SIMD3<Float>(0.24, 0.30, 0.08), bodyYaw: heading, partYaw: torsoYaw, color: SIMD4<Float>(0.18, 0.20, 0.25, 1.0)) +
            makeActorPartVertices(position: position, localCenter: SIMD3<Float>(0.0, 1.76, 0.04), halfExtents: SIMD3<Float>(0.18, 0.22, 0.18), bodyYaw: heading, partYaw: headYaw, color: SIMD4<Float>(0.80, 0.63, 0.51, 1.0)) +
            makeActorPartVertices(position: position, localCenter: SIMD3<Float>(0.0, 1.96, -0.02), halfExtents: SIMD3<Float>(0.19, 0.05, 0.09), bodyYaw: heading, partYaw: headYaw, color: SIMD4<Float>(0.18, 0.20, 0.25, 1.0)) +
            makeActorPartVertices(position: position, localCenter: SIMD3<Float>(0.0, 0.48, 0.0), halfExtents: SIMD3<Float>(0.30, 0.14, 0.16), bodyYaw: heading, partYaw: heading, color: SIMD4<Float>(0.17, 0.19, 0.24, 1.0)) +
            makeActorPartVertices(position: position, localCenter: SIMD3<Float>(-0.12, 0.22, stride), halfExtents: SIMD3<Float>(0.10, 0.22, 0.10), bodyYaw: heading, partYaw: heading, color: SIMD4<Float>(0.12, 0.14, 0.18, 1.0)) +
            makeActorPartVertices(position: position, localCenter: SIMD3<Float>(0.12, 0.22, -stride), halfExtents: SIMD3<Float>(0.10, 0.22, 0.10), bodyYaw: heading, partYaw: heading, color: SIMD4<Float>(0.12, 0.14, 0.18, 1.0)) +
            makeActorPartVertices(position: position, localCenter: SIMD3<Float>(-0.42, 1.04, -armSwing), halfExtents: SIMD3<Float>(0.08, 0.34, 0.08), bodyYaw: heading, partYaw: torsoYaw, color: SIMD4<Float>(0.91, 0.39, 0.22, 1.0)) +
            makeActorPartVertices(position: position, localCenter: SIMD3<Float>(0.42, 1.04, armSwing), halfExtents: SIMD3<Float>(0.08, 0.34, 0.08), bodyYaw: heading, partYaw: torsoYaw, color: SIMD4<Float>(0.91, 0.39, 0.22, 1.0)) +
            makeActorPartVertices(position: position, localCenter: SIMD3<Float>(0.0, 1.45, 0.18), halfExtents: SIMD3<Float>(0.22, 0.18, 0.10), bodyYaw: heading, partYaw: torsoYaw, color: SIMD4<Float>(0.86, 0.73, 0.33, 1.0))
    }

    private static func makeWorldBoxVertices(center: SIMD3<Float>, halfExtents: SIMD3<Float>, yaw: Float, color: SIMD4<Float>) -> [Vertex] {
        makeActorPartVertices(
            position: SIMD3<Float>(repeating: 0),
            localCenter: center,
            halfExtents: halfExtents,
            bodyYaw: 0,
            partYaw: yaw,
            color: color
        )
    }

    private static func makeBoxVertices(_ box: MDTBBox) -> [Vertex] {
        let center = SIMD3<Float>(box.center.x, box.center.y, box.center.z)
        let halfExtents = SIMD3<Float>(box.half_extents.x, box.half_extents.y, box.half_extents.z)
        let color = SIMD4<Float>(box.color.x, box.color.y, box.color.z, box.color.w)

        let minCorner = center - halfExtents
        let maxCorner = center + halfExtents

        let p000 = SIMD4<Float>(minCorner.x, minCorner.y, minCorner.z, 1)
        let p001 = SIMD4<Float>(minCorner.x, minCorner.y, maxCorner.z, 1)
        let p010 = SIMD4<Float>(minCorner.x, maxCorner.y, minCorner.z, 1)
        let p011 = SIMD4<Float>(minCorner.x, maxCorner.y, maxCorner.z, 1)
        let p100 = SIMD4<Float>(maxCorner.x, minCorner.y, minCorner.z, 1)
        let p101 = SIMD4<Float>(maxCorner.x, minCorner.y, maxCorner.z, 1)
        let p110 = SIMD4<Float>(maxCorner.x, maxCorner.y, minCorner.z, 1)
        let p111 = SIMD4<Float>(maxCorner.x, maxCorner.y, maxCorner.z, 1)

        let faces: [[SIMD4<Float>]] = [
            [p001, p101, p111, p001, p111, p011],
            [p100, p000, p010, p100, p010, p110],
            [p000, p001, p011, p000, p011, p010],
            [p101, p100, p110, p101, p110, p111],
            [p010, p011, p111, p010, p111, p110],
            [p000, p100, p101, p000, p101, p001],
        ]

        return faces.flatMap { face in
            face.map { position in
                Vertex(position: position, color: color)
            }
        }
    }

    private static func makeActorPartVertices(position: SIMD3<Float>, localCenter: SIMD3<Float>, halfExtents: SIMD3<Float>, bodyYaw: Float, partYaw: Float, color: SIMD4<Float>) -> [Vertex] {
        let localMin = -halfExtents
        let localMax = halfExtents
        let rotatedCenter = rotateY(localCenter, yaw: bodyYaw)

        let p000 = SIMD3<Float>(localMin.x, localMin.y, localMin.z)
        let p001 = SIMD3<Float>(localMin.x, localMin.y, localMax.z)
        let p010 = SIMD3<Float>(localMin.x, localMax.y, localMin.z)
        let p011 = SIMD3<Float>(localMin.x, localMax.y, localMax.z)
        let p100 = SIMD3<Float>(localMax.x, localMin.y, localMin.z)
        let p101 = SIMD3<Float>(localMax.x, localMin.y, localMax.z)
        let p110 = SIMD3<Float>(localMax.x, localMax.y, localMin.z)
        let p111 = SIMD3<Float>(localMax.x, localMax.y, localMax.z)

        let faces: [[SIMD3<Float>]] = [
            [p001, p101, p111, p001, p111, p011],
            [p100, p000, p010, p100, p010, p110],
            [p000, p001, p011, p000, p011, p010],
            [p101, p100, p110, p101, p110, p111],
            [p010, p011, p111, p010, p111, p110],
            [p000, p100, p101, p000, p101, p001],
        ]

        return faces.flatMap { face in
            face.map { local in
                let rotated = rotateY(local, yaw: partYaw)
                let world = position + rotatedCenter + rotated
                return Vertex(
                    position: SIMD4<Float>(world.x, world.y, world.z, 1.0),
                    color: color
                )
            }
        }
    }

    private static func wrapAngle(_ value: Float) -> Float {
        var wrapped = value
        while wrapped > Float.pi {
            wrapped -= (Float.pi * 2)
        }
        while wrapped < -Float.pi {
            wrapped += (Float.pi * 2)
        }
        return wrapped
    }

    private static func rotateY(_ point: SIMD3<Float>, yaw: Float) -> SIMD3<Float> {
        let c = cos(yaw)
        let s = sin(yaw)

        return SIMD3<Float>(
            (point.x * c) - (point.z * s),
            point.y,
            (point.x * s) + (point.z * c)
        )
    }

    private static func cameraModeLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBCameraModeThirdPerson):
            return "third-person"
        default:
            return "first-person"
        }
    }

    private static func surfaceLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBSurfaceRoad):
            return "road"
        case UInt32(MDTBSurfaceCurb):
            return "curb"
        case UInt32(MDTBSurfaceSidewalk):
            return "sidewalk"
        case UInt32(MDTBSurfaceLot):
            return "lot"
        default:
            return "unknown"
        }
    }
}
