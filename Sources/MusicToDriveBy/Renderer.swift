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

private struct SceneBlock {
    var origin: SIMD3<Float>
    var kind: UInt32
    var variant: UInt32
    var activationRadius: Float
}

private struct SceneInterestPoint {
    var position: SIMD3<Float>
    var radius: Float
    var kind: UInt32
    var blockIndex: UInt32
}

private struct SceneRoadLink {
    var fromBlockIndex: UInt32
    var toBlockIndex: UInt32
    var midpoint: SIMD3<Float>
    var length: Float
    var axis: UInt32
}

@MainActor
final class Renderer: NSObject, MTKViewDelegate {
    private static let dynamicVertexBudget = 128 * 36

    private let device: MTLDevice
    private let commandQueue: MTLCommandQueue
    private let pipelineState: MTLRenderPipelineState
    private let depthState: MTLDepthStencilState
    private let vertexBuffer: MTLBuffer
    private let sceneVertexCount: Int
    private let maxVertexCount: Int
    private let inputController: InputController
    private let blocks: [SceneBlock]
    private let roadLinks: [SceneRoadLink]
    private let interestPoints: [SceneInterestPoint]

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
        self.blocks = Renderer.loadBlocks()
        self.roadLinks = Renderer.loadRoadLinks()
        self.interestPoints = Renderer.loadInterestPoints()

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
        let ambientVertices = Self.makeAmbientVertices(
            elapsedTime: engineState.elapsed_time,
            actorPosition: actorPosition,
            activeBlockIndex: engineState.active_block_index,
            activeLinkIndex: engineState.active_link_index,
            blocks: blocks,
            roadLinks: roadLinks,
            interestPoints: interestPoints
        )
        let actorVertices = isThirdPerson
            ? Self.makeActorVertices(
                position: actorPosition,
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
        let snapshotLayout = "\(blocks.count) blocks / \(roadLinks.count) links / \(interestPoints.count) hooks"
        let snapshotActivity = Self.activitySummary(state: engineState, blocks: blocks, roadLinks: roadLinks)
        let snapshotBlock = Self.blockSummary(state: engineState, blocks: blocks)
        let snapshotNearestHook = Self.nearestInterestPointSummary(for: actorPosition, interestPoints: interestPoints, blocks: blocks)

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
            mouseLook: inputController.isMouseLookEnabled ? "captured" : "free",
            layoutSummary: snapshotLayout,
            activitySummary: snapshotActivity,
            currentBlock: snapshotBlock,
            nearestHook: snapshotNearestHook
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

    private static func loadBlocks() -> [SceneBlock] {
        let blockCount = Int(mdtb_engine_block_count())
        guard blockCount > 0 else {
            return []
        }

        let blockBuffer = UnsafeMutablePointer<MDTBBlockDescriptor>.allocate(capacity: blockCount)
        defer {
            blockBuffer.deallocate()
        }

        mdtb_engine_copy_blocks(blockBuffer, blockCount)

        return (0 ..< blockCount).map { index in
            let block = blockBuffer[index]
            return SceneBlock(
                origin: SIMD3<Float>(block.origin.x, block.origin.y, block.origin.z),
                kind: block.kind,
                variant: block.variant,
                activationRadius: block.activation_radius
            )
        }
    }

    private static func loadRoadLinks() -> [SceneRoadLink] {
        let linkCount = Int(mdtb_engine_road_link_count())
        guard linkCount > 0 else {
            return []
        }

        let linkBuffer = UnsafeMutablePointer<MDTBRoadLink>.allocate(capacity: linkCount)
        defer {
            linkBuffer.deallocate()
        }

        mdtb_engine_copy_road_links(linkBuffer, linkCount)

        return (0 ..< linkCount).map { index in
            let link = linkBuffer[index]
            return SceneRoadLink(
                fromBlockIndex: link.from_block_index,
                toBlockIndex: link.to_block_index,
                midpoint: SIMD3<Float>(link.midpoint.x, link.midpoint.y, link.midpoint.z),
                length: link.length,
                axis: link.axis
            )
        }
    }

    private static func loadInterestPoints() -> [SceneInterestPoint] {
        let pointCount = Int(mdtb_engine_interest_point_count())
        guard pointCount > 0 else {
            return []
        }

        let pointBuffer = UnsafeMutablePointer<MDTBInterestPoint>.allocate(capacity: pointCount)
        defer {
            pointBuffer.deallocate()
        }

        mdtb_engine_copy_interest_points(pointBuffer, pointCount)

        return (0 ..< pointCount).map { index in
            let point = pointBuffer[index]
            return SceneInterestPoint(
                position: SIMD3<Float>(point.position.x, point.position.y, point.position.z),
                radius: point.radius,
                kind: point.kind,
                blockIndex: point.block_index
            )
        }
    }

    private static func makeAmbientVertices(elapsedTime: Float, actorPosition: SIMD3<Float>, activeBlockIndex: UInt32, activeLinkIndex: UInt32, blocks: [SceneBlock], roadLinks: [SceneRoadLink], interestPoints: [SceneInterestPoint]) -> [Vertex] {
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
        let activeBlockIndices = Self.activeBlockIndices(
            for: actorPosition,
            activeBlockIndex: activeBlockIndex,
            activeLinkIndex: activeLinkIndex,
            blocks: blocks,
            roadLinks: roadLinks
        )
        var vertices: [Vertex] = []
        vertices.reserveCapacity((blocks.count * 16 + interestPoints.count * 8) * 36)

        for (index, block) in blocks.enumerated() where activeBlockIndices.contains(index) {
            for xSign in [-1.0 as Float, 1.0] {
                for zSign in [-1.0 as Float, 1.0] {
                    vertices.append(
                        contentsOf: makeWorldBoxVertices(
                            center: SIMD3<Float>(8.9 * xSign, 2.42, block.origin.z + (9.35 * zSign)),
                            halfExtents: SIMD3<Float>(0.14, 0.14, 0.14),
                            yaw: 0.0,
                            color: signalColor
                        )
                    )
                }
            }

            switch block.kind {
            case UInt32(MDTBBlockKindResidential):
                let busLight = 0.82 + (sin((elapsedTime * 2.2) + block.origin.z * 0.04) * 0.08)
                let windowGlow = 0.74 + (sin((elapsedTime * 1.7) + 0.6) * 0.06)
                vertices.append(
                    contentsOf: makeWorldBoxVertices(
                        center: SIMD3<Float>(11.2, 2.08, block.origin.z - 16.2),
                        halfExtents: SIMD3<Float>(0.30, 0.32, 0.05),
                        yaw: 0.0,
                        color: SIMD4<Float>(busLight, busLight, 0.92, 1.0)
                    )
                )
                vertices.append(
                    contentsOf: makeWorldBoxVertices(
                        center: SIMD3<Float>(-41.6, 2.46, block.origin.z + 15.0),
                        halfExtents: SIMD3<Float>(0.88, 0.10, 0.10),
                        yaw: 0.0,
                        color: SIMD4<Float>(0.92, 0.80, 0.34, 1.0)
                    )
                )
                vertices.append(
                    contentsOf: makeWorldBoxVertices(
                        center: SIMD3<Float>(38.4, 2.16 + (sin((elapsedTime * 1.8) + 1.1) * 0.06), block.origin.z + 33.8),
                        halfExtents: SIMD3<Float>(0.92, 0.10, 0.08),
                        yaw: 0.0,
                        color: SIMD4<Float>(windowGlow, windowGlow, 0.86, 1.0)
                    )
                )
            default:
                vertices.append(
                    contentsOf: makeWorldBoxVertices(
                        center: SIMD3<Float>(-37.15, 2.06, block.origin.z - 13.35),
                        halfExtents: SIMD3<Float>(0.84, 0.42, 0.08),
                        yaw: signYaw,
                        color: SIMD4<Float>(0.90, 0.74, 0.22, 1.0)
                    )
                )
                vertices.append(
                    contentsOf: makeWorldBoxVertices(
                        center: SIMD3<Float>(-44.9, 2.62 + (sin(pennantBase + 0.2) * 0.05), block.origin.z - 11.8),
                        halfExtents: SIMD3<Float>(0.28, 0.03, 0.12),
                        yaw: 0.0,
                        color: SIMD4<Float>(0.86, 0.36, 0.19, 1.0)
                    )
                )
                vertices.append(
                    contentsOf: makeWorldBoxVertices(
                        center: SIMD3<Float>(-42.8, 2.60 + (sin(pennantBase + 0.8) * 0.05), block.origin.z - 11.85),
                        halfExtents: SIMD3<Float>(0.28, 0.03, 0.12),
                        yaw: 0.0,
                        color: SIMD4<Float>(0.90, 0.80, 0.28, 1.0)
                    )
                )
                vertices.append(
                    contentsOf: makeWorldBoxVertices(
                        center: SIMD3<Float>(-40.7, 2.63 + (sin(pennantBase + 1.4) * 0.05), block.origin.z - 11.8),
                        halfExtents: SIMD3<Float>(0.28, 0.03, 0.12),
                        yaw: 0.0,
                        color: SIMD4<Float>(0.27, 0.54, 0.71, 1.0)
                    )
                )
                vertices.append(
                    contentsOf: makeWorldBoxVertices(
                        center: SIMD3<Float>(10.1, 2.18 + (sin((elapsedTime * 1.8) + 1.1) * 0.06), block.origin.z + 34.0),
                        halfExtents: SIMD3<Float>(0.58, 0.18, 0.06),
                        yaw: 0.0,
                        color: SIMD4<Float>(0.88, 0.91, 0.95, 1.0)
                    )
                )
            }
        }

        if activeLinkIndex < roadLinks.count {
            vertices.append(
                contentsOf: makeRoadLinkPulseVertices(
                    link: roadLinks[Int(activeLinkIndex)],
                    blocks: blocks,
                    elapsedTime: elapsedTime
                )
            )
        }

        for (pointIndex, point) in interestPoints.enumerated() {
            let blockIndex = Int(point.blockIndex)
            guard activeBlockIndices.contains(blockIndex) else {
                continue
            }

            let pointColor: SIMD4<Float>
            let halfExtents: SIMD3<Float>

            switch point.kind {
            case UInt32(MDTBInterestPointStreamingAnchor):
                pointColor = SIMD4<Float>(0.28, 0.74, 0.86, 1.0)
                halfExtents = SIMD3<Float>(0.22, 0.10, 0.22)
            case UInt32(MDTBInterestPointLandmark):
                pointColor = SIMD4<Float>(0.91, 0.67, 0.24, 1.0)
                halfExtents = SIMD3<Float>(0.18, 0.12, 0.18)
            case UInt32(MDTBInterestPointPedestrianSpawn):
                let phase = (elapsedTime * 1.9) + Float(pointIndex) * 0.7
                let offset = SIMD3<Float>(sin(phase) * 0.85, 0.0, cos(phase) * 0.42)
                let heading = atan2(offset.x, max(offset.z, -0.01))
                vertices.append(
                    contentsOf: makePedestrianPlaceholderVertices(
                        position: point.position + offset,
                        heading: heading,
                        elapsedTime: elapsedTime + Float(pointIndex) * 0.2,
                        tint: SIMD4<Float>(0.26, 0.58, 0.73, 1.0)
                    )
                )
                continue
            case UInt32(MDTBInterestPointVehicleSpawn):
                vertices.append(
                    contentsOf: makeVehiclePlaceholderVertices(
                        position: point.position,
                        yaw: vehicleYaw(for: point, blocks: blocks),
                        elapsedTime: elapsedTime + Float(pointIndex) * 0.35,
                        tint: SIMD4<Float>(0.80, 0.52, 0.23, 1.0)
                    )
                )
                continue
            default:
                continue
            }

            let bob = sin((elapsedTime * 2.0) + point.position.x * 0.08 + point.position.z * 0.03) * 0.08
            vertices.append(
                contentsOf: makeWorldBoxVertices(
                    center: SIMD3<Float>(point.position.x, point.position.y + 1.18 + bob, point.position.z),
                    halfExtents: halfExtents,
                    yaw: elapsedTime * 0.35,
                    color: pointColor
                )
            )
        }

        return vertices
    }

    private static func makePedestrianPlaceholderVertices(position: SIMD3<Float>, heading: Float, elapsedTime: Float, tint: SIMD4<Float>) -> [Vertex] {
        let gaitPhase = elapsedTime * 3.2
        let stride = sin(gaitPhase) * 0.11
        let torsoYaw = heading + (sin(gaitPhase * 0.5) * 0.12)

        return
            makeActorPartVertices(position: position, localCenter: SIMD3<Float>(0.0, 0.04, 0.0), halfExtents: SIMD3<Float>(0.24, 0.02, 0.34), bodyYaw: heading, partYaw: heading, color: SIMD4<Float>(0.10, 0.11, 0.13, 1.0)) +
            makeActorPartVertices(position: position, localCenter: SIMD3<Float>(0.0, 0.92, 0.0), halfExtents: SIMD3<Float>(0.20, 0.40, 0.14), bodyYaw: heading, partYaw: torsoYaw, color: tint) +
            makeActorPartVertices(position: position, localCenter: SIMD3<Float>(0.0, 1.45, 0.02), halfExtents: SIMD3<Float>(0.14, 0.18, 0.14), bodyYaw: heading, partYaw: torsoYaw, color: SIMD4<Float>(0.76, 0.60, 0.49, 1.0)) +
            makeActorPartVertices(position: position, localCenter: SIMD3<Float>(-0.10, 0.22, stride), halfExtents: SIMD3<Float>(0.08, 0.22, 0.08), bodyYaw: heading, partYaw: heading, color: SIMD4<Float>(0.14, 0.17, 0.21, 1.0)) +
            makeActorPartVertices(position: position, localCenter: SIMD3<Float>(0.10, 0.22, -stride), halfExtents: SIMD3<Float>(0.08, 0.22, 0.08), bodyYaw: heading, partYaw: heading, color: SIMD4<Float>(0.14, 0.17, 0.21, 1.0))
    }

    private static func makeVehiclePlaceholderVertices(position: SIMD3<Float>, yaw: Float, elapsedTime: Float, tint: SIMD4<Float>) -> [Vertex] {
        let lightPulse = 0.74 + (sin(elapsedTime * 3.6) * 0.14)
        let bodyPosition = SIMD3<Float>(position.x, position.y + 0.36, position.z)

        return
            makeWorldBoxVertices(center: bodyPosition, halfExtents: SIMD3<Float>(1.18, 0.36, 2.08), yaw: yaw, color: tint) +
            makeWorldBoxVertices(center: SIMD3<Float>(position.x, position.y + 0.74, position.z - 0.08), halfExtents: SIMD3<Float>(0.72, 0.26, 1.08), yaw: yaw, color: SIMD4<Float>(tint.x * 0.82, tint.y * 0.82, tint.z * 0.82, 1.0)) +
            makeWorldBoxVertices(center: SIMD3<Float>(position.x, position.y + 0.68, position.z + 0.88), halfExtents: SIMD3<Float>(0.62, 0.18, 0.08), yaw: yaw, color: SIMD4<Float>(0.70, 0.78, 0.84, 1.0)) +
            makeWorldBoxVertices(center: SIMD3<Float>(position.x - 0.46, position.y + 0.44, position.z + 1.98), halfExtents: SIMD3<Float>(0.12, 0.08, 0.04), yaw: yaw, color: SIMD4<Float>(lightPulse, lightPulse, 0.88, 1.0)) +
            makeWorldBoxVertices(center: SIMD3<Float>(position.x + 0.46, position.y + 0.44, position.z + 1.98), halfExtents: SIMD3<Float>(0.12, 0.08, 0.04), yaw: yaw, color: SIMD4<Float>(lightPulse, lightPulse, 0.88, 1.0))
    }

    private static func makeRoadLinkPulseVertices(link: SceneRoadLink, blocks: [SceneBlock], elapsedTime: Float) -> [Vertex] {
        guard Int(link.fromBlockIndex) < blocks.count, Int(link.toBlockIndex) < blocks.count else {
            return []
        }

        let start = blocks[Int(link.fromBlockIndex)].origin
        let end = blocks[Int(link.toBlockIndex)].origin
        let yaw: Float = link.axis == UInt32(MDTBRoadAxisNorthSouth) ? 0.0 : (.pi * 0.5)
        let pulse = 0.70 + (sin(elapsedTime * 2.6) * 0.18)
        let baseColor = SIMD4<Float>(0.24, pulse, 0.90, 1.0)
        var vertices: [Vertex] = []

        for step in 1 ... 3 {
            let t = Float(step) / 4.0
            let position = start + ((end - start) * t)
            vertices.append(
                contentsOf: makeWorldBoxVertices(
                    center: SIMD3<Float>(position.x, 0.18, position.z),
                    halfExtents: SIMD3<Float>(0.18, 0.05, 1.35),
                    yaw: yaw,
                    color: baseColor
                )
            )
        }

        return vertices
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

    private static func activeBlockIndices(for position: SIMD3<Float>, activeBlockIndex: UInt32, activeLinkIndex: UInt32, blocks: [SceneBlock], roadLinks: [SceneRoadLink]) -> Set<Int> {
        var indices = Set(
            blocks.enumerated().compactMap { index, block in
                distanceSquared(position, block.origin) <= (block.activationRadius * block.activationRadius) ? index : nil
            }
        )

        if indices.isEmpty, activeBlockIndex < blocks.count {
            indices.insert(Int(activeBlockIndex))
        }

        if activeLinkIndex < roadLinks.count {
            let link = roadLinks[Int(activeLinkIndex)]
            if link.fromBlockIndex < blocks.count {
                indices.insert(Int(link.fromBlockIndex))
            }
            if link.toBlockIndex < blocks.count {
                indices.insert(Int(link.toBlockIndex))
            }
        }

        return indices
    }

    private static func activitySummary(state: MDTBEngineState, blocks: [SceneBlock], roadLinks: [SceneRoadLink]) -> String {
        let blockLabel = blockName(for: state.active_block_index, blocks: blocks)
        let linkLabel = linkSummary(for: state.active_link_index, roadLinks: roadLinks, blocks: blocks)
        return "\(blockLabel) / \(state.nearby_block_count) nearby / \(state.active_pedestrian_spawn_count) ped / \(state.active_vehicle_spawn_count) veh / \(linkLabel)"
    }

    private static func blockSummary(state: MDTBEngineState, blocks: [SceneBlock]) -> String {
        guard state.active_block_index < blocks.count else {
            return "none"
        }

        let block = blocks[Int(state.active_block_index)]
        return "\(blockKindLabel(block.kind)) v\(block.variant) / r \(formatScalar(block.activationRadius)) / z \(formatScalar(block.origin.z))"
    }

    private static func nearestInterestPointSummary(for position: SIMD3<Float>, interestPoints: [SceneInterestPoint], blocks: [SceneBlock]) -> String {
        guard let point = interestPoints.min(by: { lhs, rhs in
            distanceSquared(position, lhs.position) < distanceSquared(position, rhs.position)
        }) else {
            return "none"
        }

        let distance = sqrt(distanceSquared(position, point.position))
        let blockLabel = blockName(for: point.blockIndex, blocks: blocks)
        return "\(interestPointLabel(point.kind)) \(formatScalar(distance))m / r \(formatScalar(point.radius)) / \(blockLabel)"
    }

    private static func distanceSquared(_ lhs: SIMD3<Float>, _ rhs: SIMD3<Float>) -> Float {
        let dx = lhs.x - rhs.x
        let dz = lhs.z - rhs.z
        return (dx * dx) + (dz * dz)
    }

    private static func formatScalar(_ value: Float) -> String {
        String(format: "%.1f", value)
    }

    private static func blockName(for index: UInt32, blocks: [SceneBlock]) -> String {
        guard index < blocks.count else {
            return "none"
        }

        return blockKindLabel(blocks[Int(index)].kind)
    }

    private static func linkSummary(for index: UInt32, roadLinks: [SceneRoadLink], blocks: [SceneBlock]) -> String {
        guard index < roadLinks.count else {
            return "no link"
        }

        let link = roadLinks[Int(index)]
        let axis = link.axis == UInt32(MDTBRoadAxisNorthSouth) ? "n-s" : "e-w"
        return "\(axis) \(blockName(for: link.fromBlockIndex, blocks: blocks))->\(blockName(for: link.toBlockIndex, blocks: blocks))"
    }

    private static func vehicleYaw(for point: SceneInterestPoint, blocks: [SceneBlock]) -> Float {
        let blockOriginZ: Float
        if point.blockIndex < blocks.count {
            blockOriginZ = blocks[Int(point.blockIndex)].origin.z
        } else {
            blockOriginZ = point.position.z
        }

        if abs(point.position.x) <= 18.0 {
            return .pi * 0.5
        }

        if abs(point.position.z - blockOriginZ) <= 18.0 {
            return 0.0
        }

        return point.position.x >= 0.0 ? 0.0 : .pi
    }

    private static func blockKindLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBBlockKindResidential):
            return "residential"
        case UInt32(MDTBBlockKindMixedUse):
            return "mixed-use"
        default:
            return "hub"
        }
    }

    private static func interestPointLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBInterestPointPedestrianSpawn):
            return "ped spawn"
        case UInt32(MDTBInterestPointVehicleSpawn):
            return "vehicle spawn"
        case UInt32(MDTBInterestPointLandmark):
            return "landmark"
        case UInt32(MDTBInterestPointStreamingAnchor):
            return "stream anchor"
        default:
            return "hook"
        }
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
