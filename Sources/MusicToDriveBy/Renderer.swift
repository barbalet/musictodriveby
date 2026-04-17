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
    var district: UInt32
    var tagMask: UInt32
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

private struct SceneDynamicProp {
    var position: SIMD3<Float>
    var halfExtents: SIMD3<Float>
    var color: SIMD4<Float>
    var phaseOffset: Float
    var kind: UInt32
    var blockIndex: UInt32
}

private struct PopulationActivity {
    var livePedestrians: Int
    var liveVehicles: Int
}

private struct PedestrianSample {
    var position: SIMD3<Float>
    var heading: Float
    var tint: SIMD4<Float>
}

private struct VehicleSample {
    var position: SIMD3<Float>
    var yaw: Float
    var tint: SIMD4<Float>
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
    private let dynamicProps: [SceneDynamicProp]

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
        self.dynamicProps = Renderer.loadDynamicProps()

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
            interestPoints: interestPoints,
            dynamicProps: dynamicProps
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
        let snapshotLayout = "\(blocks.count) blocks / \(roadLinks.count) links / \(interestPoints.count) hooks / \(dynamicProps.count) dynamic"
        let snapshotActivity = Self.activitySummary(state: engineState, blocks: blocks, roadLinks: roadLinks, interestPoints: interestPoints)
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
                activationRadius: block.activation_radius,
                district: block.district,
                tagMask: block.tag_mask
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

    private static func loadDynamicProps() -> [SceneDynamicProp] {
        let propCount = Int(mdtb_engine_dynamic_prop_count())
        guard propCount > 0 else {
            return []
        }

        let propBuffer = UnsafeMutablePointer<MDTBDynamicProp>.allocate(capacity: propCount)
        defer {
            propBuffer.deallocate()
        }

        mdtb_engine_copy_dynamic_props(propBuffer, propCount)

        return (0 ..< propCount).map { index in
            let prop = propBuffer[index]
            return SceneDynamicProp(
                position: SIMD3<Float>(prop.position.x, prop.position.y, prop.position.z),
                halfExtents: SIMD3<Float>(prop.half_extents.x, prop.half_extents.y, prop.half_extents.z),
                color: SIMD4<Float>(prop.color.x, prop.color.y, prop.color.z, prop.color.w),
                phaseOffset: prop.phase_offset,
                kind: prop.kind,
                blockIndex: prop.block_index
            )
        }
    }

    private static func makeAmbientVertices(elapsedTime: Float, actorPosition: SIMD3<Float>, activeBlockIndex: UInt32, activeLinkIndex: UInt32, blocks: [SceneBlock], roadLinks: [SceneRoadLink], interestPoints: [SceneInterestPoint], dynamicProps: [SceneDynamicProp]) -> [Vertex] {
        let activeBlockIndices = Self.activeBlockIndices(
            for: actorPosition,
            activeBlockIndex: activeBlockIndex,
            activeLinkIndex: activeLinkIndex,
            blocks: blocks,
            roadLinks: roadLinks
        )
        var vertices: [Vertex] = []
        vertices.reserveCapacity((dynamicProps.count + interestPoints.count * 2 + roadLinks.count * 3) * 36)

        for dynamicProp in dynamicProps where activeBlockIndices.contains(Int(dynamicProp.blockIndex)) {
            vertices.append(contentsOf: makeDynamicPropVertices(dynamicProp, elapsedTime: elapsedTime))
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
                if let block = blockIndex < blocks.count ? blocks[blockIndex] : nil,
                   let sample = pedestrianSample(for: point, pointIndex: pointIndex, block: block, elapsedTime: elapsedTime) {
                    vertices.append(
                        contentsOf: makePedestrianPlaceholderVertices(
                            position: sample.position,
                            heading: sample.heading,
                            elapsedTime: elapsedTime + Float(pointIndex) * 0.2,
                            tint: sample.tint
                        )
                    )
                }
                continue
            case UInt32(MDTBInterestPointVehicleSpawn):
                if let block = blockIndex < blocks.count ? blocks[blockIndex] : nil,
                   let sample = vehicleSample(for: point, pointIndex: pointIndex, block: block, elapsedTime: elapsedTime) {
                    vertices.append(
                        contentsOf: makeVehiclePlaceholderVertices(
                            position: sample.position,
                            yaw: sample.yaw,
                            elapsedTime: elapsedTime + Float(pointIndex) * 0.35,
                            tint: sample.tint
                        )
                    )
                }
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

    private static func makeDynamicPropVertices(_ dynamicProp: SceneDynamicProp, elapsedTime: Float) -> [Vertex] {
        var center = dynamicProp.position
        var color = dynamicProp.color
        var yaw: Float = 0.0

        switch dynamicProp.kind {
        case UInt32(MDTBDynamicPropSignalLamp):
            let signalPhase = (elapsedTime + dynamicProp.phaseOffset).truncatingRemainder(dividingBy: 8.0)
            switch signalPhase {
            case 0 ..< 3.0:
                color = SIMD4<Float>(0.22, 0.88, 0.31, 1.0)
            case 3.0 ..< 4.2:
                color = SIMD4<Float>(0.93, 0.74, 0.16, 1.0)
            default:
                color = SIMD4<Float>(0.88, 0.22, 0.18, 1.0)
            }
        case UInt32(MDTBDynamicPropSwingSign):
            yaw = sin((elapsedTime * 1.35) + dynamicProp.phaseOffset) * 0.24
        case UInt32(MDTBDynamicPropPennant):
            center.y += sin((elapsedTime * 2.8) + dynamicProp.phaseOffset) * 0.05
        case UInt32(MDTBDynamicPropWindowGlow):
            center.y += sin((elapsedTime * 1.8) + dynamicProp.phaseOffset) * 0.06
            color = animatedColor(dynamicProp.color, intensity: 0.82 + (sin((elapsedTime * 1.6) + dynamicProp.phaseOffset) * 0.10))
        case UInt32(MDTBDynamicPropTransitGlow):
            color = animatedColor(dynamicProp.color, intensity: 0.88 + (sin((elapsedTime * 2.2) + dynamicProp.phaseOffset) * 0.10))
        case UInt32(MDTBDynamicPropNeon):
            color = animatedColor(dynamicProp.color, intensity: 0.90 + (sin((elapsedTime * 3.0) + dynamicProp.phaseOffset) * 0.12))
        default:
            break
        }

        return makeWorldBoxVertices(center: center, halfExtents: dynamicProp.halfExtents, yaw: yaw, color: color)
    }

    private static func pedestrianSample(for point: SceneInterestPoint, pointIndex: Int, block: SceneBlock, elapsedTime: Float) -> PedestrianSample? {
        let districtTempo: Float
        let activeShare: Float

        switch block.district {
        case UInt32(MDTBDistrictMarketSpur):
            districtTempo = 1.15
            activeShare = 0.76
        case UInt32(MDTBDistrictMapleHeights):
            districtTempo = 0.86
            activeShare = 0.54
        default:
            districtTempo = 1.0
            activeShare = 0.66
        }

        let cycleLength: Float = 8.5 + (Float(pointIndex % 3) * 1.35)
        let seed = (Float(pointIndex) * 1.37) + (Float(block.variant) * 0.61)
        let cycle = ((elapsedTime * districtTempo) + seed).truncatingRemainder(dividingBy: cycleLength)
        let activeWindow = cycleLength * activeShare
        guard cycle < activeWindow else {
            return nil
        }

        let progress = cycle / max(activeWindow, 0.01)
        let loop = (progress * (.pi * 2.0)) + seed
        let radiusX: Float = (block.tagMask & UInt32(MDTBBlockTagRetail)) != 0 ? 1.05 : 0.78
        let radiusZ: Float = (block.tagMask & UInt32(MDTBBlockTagResidential)) != 0 ? 0.72 : 0.48
        let offset = SIMD3<Float>(
            sin(loop) * radiusX,
            0.0,
            sin((loop * 0.5) + 0.3) * cos(loop) * radiusZ
        )
        let tangent = SIMD3<Float>(
            cos(loop) * radiusX,
            0.0,
            ((cos((loop * 0.5) + 0.3) * 0.5 * cos(loop)) - (sin((loop * 0.5) + 0.3) * sin(loop))) * radiusZ
        )

        return PedestrianSample(
            position: point.position + offset,
            heading: atan2(tangent.x, -tangent.z),
            tint: pedestrianTint(for: block)
        )
    }

    private static func vehicleSample(for point: SceneInterestPoint, pointIndex: Int, block: SceneBlock, elapsedTime: Float) -> VehicleSample? {
        let pace: Float = (block.tagMask & UInt32(MDTBBlockTagRetail)) != 0 ? 1.0 : 0.82
        let activeShare: Float = block.district == UInt32(MDTBDistrictMapleHeights) ? 0.42 : 0.60
        let cycleLength: Float = 11.5 + Float(pointIndex % 2) * 2.4
        let seed = (Float(pointIndex) * 0.83) + (Float(block.variant) * 0.47)
        let cycle = ((elapsedTime * pace) + seed).truncatingRemainder(dividingBy: cycleLength)
        let activeWindow = cycleLength * activeShare
        guard cycle < activeWindow else {
            return nil
        }

        let progress = smoothstep(cycle / max(activeWindow, 0.01))
        let yaw = vehicleYaw(for: point, block: block)
        let forward = SIMD3<Float>(sin(yaw), 0.0, -cos(yaw))
        let travelSpan: Float = (block.tagMask & UInt32(MDTBBlockTagSpur)) != 0 ? 7.5 : 5.5
        let laneOffset = forward * ((progress - 0.5) * travelSpan)

        return VehicleSample(
            position: point.position + laneOffset,
            yaw: yaw,
            tint: vehicleTint(for: block)
        )
    }

    private static func pedestrianTint(for block: SceneBlock) -> SIMD4<Float> {
        switch block.district {
        case UInt32(MDTBDistrictMarketSpur):
            return SIMD4<Float>(0.31, 0.62, 0.78, 1.0)
        case UInt32(MDTBDistrictMapleHeights):
            return SIMD4<Float>(0.44, 0.60, 0.34, 1.0)
        default:
            return SIMD4<Float>(0.82, 0.44, 0.24, 1.0)
        }
    }

    private static func vehicleTint(for block: SceneBlock) -> SIMD4<Float> {
        switch block.district {
        case UInt32(MDTBDistrictMarketSpur):
            return SIMD4<Float>(0.83, 0.54, 0.22, 1.0)
        case UInt32(MDTBDistrictMapleHeights):
            return SIMD4<Float>(0.34, 0.52, 0.70, 1.0)
        default:
            return SIMD4<Float>(0.72, 0.42, 0.26, 1.0)
        }
    }

    private static func animatedColor(_ base: SIMD4<Float>, intensity: Float) -> SIMD4<Float> {
        let clampedIntensity = max(0.25, intensity)
        return SIMD4<Float>(
            min(base.x * clampedIntensity, 1.0),
            min(base.y * clampedIntensity, 1.0),
            min(base.z * clampedIntensity, 1.0),
            base.w
        )
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

    private static func activitySummary(state: MDTBEngineState, blocks: [SceneBlock], roadLinks: [SceneRoadLink], interestPoints: [SceneInterestPoint]) -> String {
        let blockLabel = blockName(for: state.active_block_index, blocks: blocks)
        let linkLabel = linkSummary(for: state.active_link_index, roadLinks: roadLinks, blocks: blocks)
        let population = populationActivity(state: state, blocks: blocks, roadLinks: roadLinks, interestPoints: interestPoints)
        return "\(blockLabel) / \(state.nearby_block_count) nearby / \(population.livePedestrians) live ped / \(population.liveVehicles) live veh / \(linkLabel)"
    }

    private static func blockSummary(state: MDTBEngineState, blocks: [SceneBlock]) -> String {
        guard state.active_block_index < blocks.count else {
            return "none"
        }

        let block = blocks[Int(state.active_block_index)]
        return "\(blockKindLabel(block.kind)) / \(districtLabel(block.district)) / \(tagLabel(block.tagMask)) / r \(formatScalar(block.activationRadius))"
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

    private static func populationActivity(state: MDTBEngineState, blocks: [SceneBlock], roadLinks: [SceneRoadLink], interestPoints: [SceneInterestPoint]) -> PopulationActivity {
        guard !interestPoints.isEmpty else {
            return PopulationActivity(livePedestrians: 0, liveVehicles: 0)
        }

        let actorPosition = SIMD3<Float>(
            state.actor_position.x,
            state.actor_position.y,
            state.actor_position.z
        )
        let activeBlockIndices = activeBlockIndices(
            for: actorPosition,
            activeBlockIndex: state.active_block_index,
            activeLinkIndex: state.active_link_index,
            blocks: blocks,
            roadLinks: roadLinks
        )

        var pedestrians = 0
        var vehicles = 0

        for (pointIndex, point) in interestPoints.enumerated() {
            let blockIndex = Int(point.blockIndex)
            guard blockIndex < blocks.count, activeBlockIndices.contains(blockIndex) else {
                continue
            }

            let block = blocks[blockIndex]
            switch point.kind {
            case UInt32(MDTBInterestPointPedestrianSpawn):
                if pedestrianSample(for: point, pointIndex: pointIndex, block: block, elapsedTime: state.elapsed_time) != nil {
                    pedestrians += 1
                }
            case UInt32(MDTBInterestPointVehicleSpawn):
                if vehicleSample(for: point, pointIndex: pointIndex, block: block, elapsedTime: state.elapsed_time) != nil {
                    vehicles += 1
                }
            default:
                break
            }
        }

        return PopulationActivity(livePedestrians: pedestrians, liveVehicles: vehicles)
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

    private static func vehicleYaw(for point: SceneInterestPoint, block: SceneBlock) -> Float {
        if abs(point.position.x) <= 18.0 {
            return .pi * 0.5
        }

        if abs(point.position.z - block.origin.z) <= 18.0 {
            return 0.0
        }

        return point.position.x >= 0.0 ? 0.0 : .pi
    }

    private static func districtLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBDistrictMapleHeights):
            return "maple-heights"
        case UInt32(MDTBDistrictMarketSpur):
            return "market-spur"
        default:
            return "south-hub"
        }
    }

    private static func tagLabel(_ value: UInt32) -> String {
        var labels: [String] = []

        if (value & UInt32(MDTBBlockTagRetail)) != 0 {
            labels.append("retail")
        }
        if (value & UInt32(MDTBBlockTagTransit)) != 0 {
            labels.append("transit")
        }
        if (value & UInt32(MDTBBlockTagLandmark)) != 0 {
            labels.append("landmark")
        }
        if (value & UInt32(MDTBBlockTagCourt)) != 0 {
            labels.append("court")
        }
        if (value & UInt32(MDTBBlockTagResidential)) != 0 {
            labels.append("res")
        }
        if (value & UInt32(MDTBBlockTagSpur)) != 0 {
            labels.append("spur")
        }

        return labels.isEmpty ? "untagged" : labels.joined(separator: "+")
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

    private static func smoothstep(_ value: Float) -> Float {
        let clamped = max(0.0, min(value, 1.0))
        return clamped * clamped * (3.0 - (2.0 * clamped))
    }
}
