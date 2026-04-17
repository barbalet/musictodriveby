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
    var frontageTemplate: UInt32
    var chunkIndex: UInt32
}

private struct SceneStaticBox {
    var box: MDTBBox
    var blockIndex: UInt32
    var layer: UInt32
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

private struct SceneVehicleAnchor {
    var position: SIMD3<Float>
    var yaw: Float
    var blockIndex: UInt32
    var kind: UInt32
    var parkingState: UInt32
    var laneAxis: UInt32
    var laneOffset: Float
}

private struct SceneDynamicProp {
    var position: SIMD3<Float>
    var halfExtents: SIMD3<Float>
    var color: SIMD4<Float>
    var phaseOffset: Float
    var kind: UInt32
    var blockIndex: UInt32
}

private struct ScenePopulationProfile {
    var blockIndex: UInt32
    var pedestrianDensity: Float
    var vehicleDensity: Float
    var ambientEnergy: Float
    var travelBias: Float
    var styleFlags: UInt32
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
    var kind: UInt32
}

private struct RouteSample {
    var position: SIMD3<Float>
    var heading: Float
}

@MainActor
final class Renderer: NSObject, MTKViewDelegate {
    private static let dynamicVertexBudget = 256 * 36

    private let device: MTLDevice
    private let commandQueue: MTLCommandQueue
    private let pipelineState: MTLRenderPipelineState
    private let depthState: MTLDepthStencilState
    private let vertexBuffer: MTLBuffer
    private let maxVertexCount: Int
    private let inputController: InputController
    private let staticSceneBoxes: [SceneStaticBox]
    private let blocks: [SceneBlock]
    private let roadLinks: [SceneRoadLink]
    private let interestPoints: [SceneInterestPoint]
    private let dynamicProps: [SceneDynamicProp]
    private let populationProfiles: [ScenePopulationProfile]

    private weak var viewModel: GameViewModel?
    private var engineState = MDTBEngineState()
    private var lastFrameTime = CACurrentMediaTime()
    private var fpsWindowTime = 0.0
    private var fpsWindowFrames = 0
    private var latestFPS = 60.0
    private var latestVisibleStaticBoxCount = 0

    init(view: MTKView, inputController: InputController, viewModel: GameViewModel) {
        guard let commandQueue = view.device?.makeCommandQueue() else {
            preconditionFailure("Unable to create Metal command queue.")
        }

        self.device = view.device!
        self.commandQueue = commandQueue
        self.inputController = inputController
        self.viewModel = viewModel
        mdtb_engine_init(&engineState)
        self.staticSceneBoxes = Renderer.loadStaticSceneBoxes()
        self.blocks = Renderer.loadBlocks()
        self.roadLinks = Renderer.loadRoadLinks()
        self.interestPoints = Renderer.loadInterestPoints()
        self.dynamicProps = Renderer.loadDynamicProps()
        self.populationProfiles = Renderer.loadPopulationProfiles()

        let vertexDescriptor = Renderer.makeVertexDescriptor()
        self.pipelineState = Renderer.makePipelineState(device: self.device, pixelFormat: view.colorPixelFormat, depthFormat: view.depthStencilPixelFormat, vertexDescriptor: vertexDescriptor)
        self.depthState = Renderer.makeDepthState(device: self.device)

        self.maxVertexCount = (staticSceneBoxes.count * 36) + Self.dynamicVertexBudget

        guard let vertexBuffer = self.device.makeBuffer(length: MemoryLayout<Vertex>.stride * maxVertexCount, options: .storageModeShared) else {
            preconditionFailure("Unable to allocate vertex buffer.")
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
        let vehicleAnchors = Self.loadVehicleAnchors()

        fpsWindowTime += Double(deltaTime)
        fpsWindowFrames += 1
        if fpsWindowTime >= 0.25 {
            latestFPS = Double(fpsWindowFrames) / fpsWindowTime
            fpsWindowTime = 0
            fpsWindowFrames = 0
            publishDebugState(vehicleAnchors: vehicleAnchors)
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
        let isVehicleTraversal = engineState.traversal_mode == UInt32(MDTBTraversalModeVehicle)
        let activeBlockIndices = Self.activeBlockIndices(
            for: actorPosition,
            activeBlockIndex: engineState.active_block_index,
            activeLinkIndex: engineState.active_link_index,
            blocks: blocks,
            roadLinks: roadLinks
        )
        let activeChunkIndices = Self.activeChunkIndices(activeBlockIndices: activeBlockIndices, blocks: blocks)
        let visibleBlockIndices = Self.visibleBlockIndices(activeChunkIndices: activeChunkIndices, blocks: blocks)
        let staticVertices = Self.makeStaticSceneVertices(
            staticSceneBoxes: staticSceneBoxes,
            blocks: blocks,
            activeChunkIndices: activeChunkIndices
        )
        let ambientVertices = Self.makeAmbientVertices(
            elapsedTime: engineState.elapsed_time,
            activeLinkIndex: engineState.active_link_index,
            visibleBlockIndices: visibleBlockIndices,
            blocks: blocks,
            roadLinks: roadLinks,
            interestPoints: interestPoints,
            dynamicProps: dynamicProps,
            populationProfiles: populationProfiles
        )
        let vehicleAnchorVertices = Self.makeVehicleAnchorVertices(
            vehicleAnchors: vehicleAnchors,
            visibleBlockIndices: visibleBlockIndices,
            state: engineState,
            elapsedTime: engineState.elapsed_time,
            blocks: blocks
        )
        let actorVertices = isThirdPerson && engineState.traversal_mode == UInt32(MDTBTraversalModeOnFoot)
            ? Self.makeActorVertices(
                position: actorPosition,
                heading: engineState.actor_heading,
                cameraYaw: engineState.camera.yaw,
                speed: engineState.camera.move_speed,
                elapsedTime: engineState.elapsed_time
            )
            : []
        let drawVertices = staticVertices + ambientVertices + vehicleAnchorVertices + actorVertices
        let drawVertexCount = drawVertices.count
        latestVisibleStaticBoxCount = staticVertices.count / 36

        if drawVertexCount > maxVertexCount {
            preconditionFailure("Vertex budget exceeded for current frame.")
        }

        if !drawVertices.isEmpty {
            _ = drawVertices.withUnsafeBytes { bytes in
                memcpy(vertexBuffer.contents(), bytes.baseAddress, bytes.count)
            }
        }

        let drawableWidth = max(Float(view.drawableSize.width), 1)
        let drawableHeight = max(Float(view.drawableSize.height), 1)
        let aspect = drawableWidth / drawableHeight

        let projection = simd_float4x4.perspectiveProjection(
            fovY: (isThirdPerson ? (isVehicleTraversal ? 68 : 70) : (isVehicleTraversal ? 78 : 74)) * (.pi / 180),
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

    private func publishDebugState(vehicleAnchors: [SceneVehicleAnchor]) {
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
        let snapshotCameraMode = Self.cameraModeLabel(engineState.camera.mode, traversalMode: engineState.traversal_mode)
        let snapshotSurface = Self.surfaceLabel(engineState.surface_kind)
        let snapshotLayout = "\(blocks.count) blocks / \(Self.chunkCount(blocks)) chunks / \(roadLinks.count) links / \(vehicleAnchors.count) staged veh / \(interestPoints.count) hooks / \(dynamicProps.count) dynamic / \(latestVisibleStaticBoxCount)/\(staticSceneBoxes.count) static"
        let snapshotActivity = Self.activitySummary(state: engineState, blocks: blocks, roadLinks: roadLinks, interestPoints: interestPoints, populationProfiles: populationProfiles, vehicleAnchors: vehicleAnchors)
        let snapshotVehicle = Self.vehicleStatusSummary(state: engineState, vehicleAnchors: vehicleAnchors, blocks: blocks)
        let snapshotInteraction = Self.interactionSummary(state: engineState, vehicleAnchors: vehicleAnchors, blocks: blocks)
        let snapshotBlock = Self.blockSummary(state: engineState, blocks: blocks, populationProfiles: populationProfiles)
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
            vehicleStatus: snapshotVehicle,
            interactionSummary: snapshotInteraction,
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

    private static func loadStaticSceneBoxes() -> [SceneStaticBox] {
        let boxCount = Int(mdtb_engine_scene_box_count())
        guard boxCount > 0 else {
            return []
        }

        let boxBuffer = UnsafeMutablePointer<MDTBSceneBox>.allocate(capacity: boxCount)
        defer {
            boxBuffer.deallocate()
        }

        mdtb_engine_copy_scene_boxes(boxBuffer, boxCount)

        return (0 ..< boxCount).map { index in
            let sceneBox = boxBuffer[index]
            return SceneStaticBox(
                box: sceneBox.box,
                blockIndex: sceneBox.block_index,
                layer: sceneBox.layer
            )
        }
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
                tagMask: block.tag_mask,
                frontageTemplate: block.frontage_template,
                chunkIndex: block.chunk_index
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

    private static func loadVehicleAnchors() -> [SceneVehicleAnchor] {
        let anchorCount = Int(mdtb_engine_vehicle_anchor_count())
        guard anchorCount > 0 else {
            return []
        }

        let anchorBuffer = UnsafeMutablePointer<MDTBVehicleAnchor>.allocate(capacity: anchorCount)
        defer {
            anchorBuffer.deallocate()
        }

        mdtb_engine_copy_vehicle_anchors(anchorBuffer, anchorCount)

        return (0 ..< anchorCount).map { index in
            let anchor = anchorBuffer[index]
            return SceneVehicleAnchor(
                position: SIMD3<Float>(anchor.position.x, anchor.position.y, anchor.position.z),
                yaw: anchor.yaw,
                blockIndex: anchor.block_index,
                kind: anchor.kind,
                parkingState: anchor.parking_state,
                laneAxis: anchor.lane_axis,
                laneOffset: anchor.lane_offset
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

    private static func loadPopulationProfiles() -> [ScenePopulationProfile] {
        let profileCount = Int(mdtb_engine_population_profile_count())
        guard profileCount > 0 else {
            return []
        }

        let profileBuffer = UnsafeMutablePointer<MDTBPopulationProfile>.allocate(capacity: profileCount)
        defer {
            profileBuffer.deallocate()
        }

        mdtb_engine_copy_population_profiles(profileBuffer, profileCount)

        return (0 ..< profileCount).map { index in
            let profile = profileBuffer[index]
            return ScenePopulationProfile(
                blockIndex: profile.block_index,
                pedestrianDensity: profile.pedestrian_density,
                vehicleDensity: profile.vehicle_density,
                ambientEnergy: profile.ambient_energy,
                travelBias: profile.travel_bias,
                styleFlags: profile.style_flags
            )
        }
    }

    private static func makeStaticSceneVertices(staticSceneBoxes: [SceneStaticBox], blocks: [SceneBlock], activeChunkIndices: Set<Int>) -> [Vertex] {
        var vertices: [Vertex] = []
        vertices.reserveCapacity(staticSceneBoxes.count * 36)

        for sceneBox in staticSceneBoxes {
            if sceneBox.layer == UInt32(MDTBSceneLayerBlockOwned) {
                let blockIndex = Int(sceneBox.blockIndex)
                guard blockIndex < blocks.count else {
                    continue
                }

                if !activeChunkIndices.contains(Int(blocks[blockIndex].chunkIndex)) {
                    continue
                }
            }

            if sceneBox.layer != UInt32(MDTBSceneLayerShared) && sceneBox.layer != UInt32(MDTBSceneLayerBlockOwned) {
                continue
            }

            vertices.append(contentsOf: makeBoxVertices(sceneBox.box))
        }

        return vertices
    }

    private static func makeAmbientVertices(elapsedTime: Float, activeLinkIndex: UInt32, visibleBlockIndices: Set<Int>, blocks: [SceneBlock], roadLinks: [SceneRoadLink], interestPoints: [SceneInterestPoint], dynamicProps: [SceneDynamicProp], populationProfiles: [ScenePopulationProfile]) -> [Vertex] {
        var vertices: [Vertex] = []
        vertices.reserveCapacity((dynamicProps.count + interestPoints.count * 2 + roadLinks.count * 3 + visibleBlockIndices.count * 8) * 36)

        for blockIndex in visibleBlockIndices.sorted() where blockIndex < blocks.count {
            let block = blocks[blockIndex]
            let profile = populationProfile(for: blockIndex, block: block, populationProfiles: populationProfiles)
            vertices.append(contentsOf: makeDistrictAmbientVertices(block: block, profile: profile, elapsedTime: elapsedTime))
        }

        for dynamicProp in dynamicProps where visibleBlockIndices.contains(Int(dynamicProp.blockIndex)) {
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
            guard visibleBlockIndices.contains(blockIndex), blockIndex < blocks.count else {
                continue
            }

            let block = blocks[blockIndex]
            let profile = populationProfile(for: blockIndex, block: block, populationProfiles: populationProfiles)
            let pointColor: SIMD4<Float>
            let halfExtents: SIMD3<Float>

            switch point.kind {
            case UInt32(MDTBInterestPointStreamingAnchor):
                pointColor = SIMD4<Float>(0.28, 0.74, 0.86, 1.0)
                halfExtents = SIMD3<Float>(0.22, 0.10, 0.22)
            case UInt32(MDTBInterestPointLandmark):
                pointColor = SIMD4<Float>(0.91, 0.67, 0.24, 1.0)
                halfExtents = SIMD3<Float>(0.18, 0.12, 0.18)
            case UInt32(MDTBInterestPointHotspot):
                pointColor = animatedColor(districtAmbientColor(for: block), intensity: 0.92 + profile.ambientEnergy * 0.18)
                halfExtents = SIMD3<Float>(0.28, 0.16, 0.28)
            case UInt32(MDTBInterestPointPedestrianSpawn):
                if let sample = pedestrianSample(
                    for: point,
                    pointIndex: pointIndex,
                    block: block,
                    profile: populationProfile(for: blockIndex, block: block, populationProfiles: populationProfiles),
                    blocks: blocks,
                    roadLinks: roadLinks,
                    interestPoints: interestPoints,
                    elapsedTime: elapsedTime
                ) {
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
                if let sample = vehicleSample(
                    for: point,
                    pointIndex: pointIndex,
                    block: block,
                    profile: populationProfile(for: blockIndex, block: block, populationProfiles: populationProfiles),
                    blocks: blocks,
                    roadLinks: roadLinks,
                    elapsedTime: elapsedTime
                ) {
                    vertices.append(
                        contentsOf: makeVehiclePlaceholderVertices(
                            position: sample.position,
                            yaw: sample.yaw,
                            elapsedTime: elapsedTime + Float(pointIndex) * 0.35,
                            tint: sample.tint,
                            kind: sample.kind
                        )
                    )
                }
                continue
            default:
                continue
            }

            let bob = sin((elapsedTime * 2.0) + point.position.x * 0.08 + point.position.z * 0.03) * (point.kind == UInt32(MDTBInterestPointHotspot) ? 0.12 : 0.08)
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

    private static func makeVehicleAnchorVertices(vehicleAnchors: [SceneVehicleAnchor], visibleBlockIndices: Set<Int>, state: MDTBEngineState, elapsedTime: Float, blocks: [SceneBlock]) -> [Vertex] {
        var vertices: [Vertex] = []

        for (anchorIndex, anchor) in vehicleAnchors.enumerated() {
            let blockIndex = Int(anchor.blockIndex)
            guard visibleBlockIndices.contains(blockIndex) else {
                continue
            }

            let block = blocks[safe: blockIndex]
            var tint = block.map(vehicleTint(for:)) ?? SIMD4<Float>(0.72, 0.42, 0.26, 1.0)
            let isActiveVehicle = state.traversal_mode == UInt32(MDTBTraversalModeVehicle) && state.active_vehicle_anchor_index == UInt32(anchorIndex)
            let isNearbyVehicle = state.traversal_mode == UInt32(MDTBTraversalModeOnFoot) && state.nearby_vehicle_anchor_index == UInt32(anchorIndex)

            if isActiveVehicle {
                tint = animatedColor(tint, intensity: 1.18)
            } else if isNearbyVehicle {
                tint = animatedColor(tint, intensity: 1.06)
            } else {
                tint = animatedColor(tint, intensity: 0.88)
            }

            vertices.append(
                contentsOf: makeVehiclePlaceholderVertices(
                    position: anchor.position,
                    yaw: anchor.yaw,
                    elapsedTime: elapsedTime + Float(anchorIndex) * 0.4,
                    tint: tint,
                    kind: anchor.kind
                )
            )

            vertices.append(
                contentsOf: makeVehicleParkingGuideVertices(
                    anchor: anchor,
                    elapsedTime: elapsedTime,
                    isActive: isActiveVehicle,
                    isNearby: isNearbyVehicle
                )
            )

            if isActiveVehicle || isNearbyVehicle {
                let beaconColor = isActiveVehicle
                    ? SIMD4<Float>(0.24, 0.84, 0.98, 1.0)
                    : SIMD4<Float>(0.98, 0.82, 0.24, 1.0)
                let bob = sin((elapsedTime * 3.2) + Float(anchorIndex) * 0.35) * 0.08
                vertices.append(
                    contentsOf: makeWorldBoxVertices(
                        center: SIMD3<Float>(anchor.position.x, anchor.position.y + 1.52 + bob, anchor.position.z),
                        halfExtents: SIMD3<Float>(0.18, 0.10, 0.18),
                        yaw: elapsedTime * 0.45,
                        color: beaconColor
                    )
                )
            }
        }

        return vertices
    }

    private static func makeVehicleParkingGuideVertices(anchor: SceneVehicleAnchor, elapsedTime: Float, isActive: Bool, isNearby: Bool) -> [Vertex] {
        let intensity: Float = isActive ? 1.04 : (isNearby ? 0.92 : 0.72)
        let color: SIMD4<Float> = anchor.parkingState == UInt32(MDTBVehicleParkingStateService)
            ? animatedColor(SIMD4<Float>(0.84, 0.58, 0.22, 1.0), intensity: intensity)
            : animatedColor(SIMD4<Float>(0.78, 0.82, 0.88, 1.0), intensity: intensity)
        let bob = sin((elapsedTime * 2.4) + anchor.position.x * 0.03 + anchor.position.z * 0.05) * (isActive ? 0.02 : 0.0)
        let guideCenter = anchor.position.y + 0.04 + bob

        if anchor.laneAxis == UInt32(MDTBRoadAxisNorthSouth) {
            return makeWorldBoxVertices(
                center: SIMD3<Float>(anchor.position.x, guideCenter, anchor.position.z),
                halfExtents: SIMD3<Float>(0.18, 0.01, anchor.parkingState == UInt32(MDTBVehicleParkingStateService) ? 2.30 : 2.65),
                yaw: 0.0,
                color: color
            )
        }

        return makeWorldBoxVertices(
            center: SIMD3<Float>(anchor.position.x, guideCenter, anchor.position.z),
            halfExtents: SIMD3<Float>(anchor.parkingState == UInt32(MDTBVehicleParkingStateService) ? 2.30 : 2.65, 0.01, 0.18),
            yaw: .pi * 0.5,
            color: color
        )
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

    private static func makeDistrictAmbientVertices(block: SceneBlock, profile: ScenePopulationProfile, elapsedTime: Float) -> [Vertex] {
        let energy = max(profile.ambientEnergy, 0.25)
        let baseColor = districtAmbientColor(for: block)
        let pulse = 0.74 + (sin((elapsedTime * (1.2 + energy)) + Float(block.variant) * 0.5) * 0.14 * energy)
        var vertices: [Vertex] = []

        if (block.tagMask & UInt32(MDTBBlockTagRetail)) != 0 {
            vertices.append(
                contentsOf: makeWorldBoxVertices(
                    center: SIMD3<Float>(block.origin.x + 6.0, block.origin.y + 2.26, block.origin.z - 13.0),
                    halfExtents: SIMD3<Float>(2.8, 0.12, 0.08),
                    yaw: 0.0,
                    color: animatedColor(baseColor, intensity: pulse)
                )
            )
        }

        if (block.tagMask & UInt32(MDTBBlockTagTransit)) != 0 {
            let transitColor = animatedColor(baseColor, intensity: 0.82 + energy * 0.18)
            vertices.append(contentsOf: makeWorldBoxVertices(center: SIMD3<Float>(block.origin.x - 10.2, block.origin.y + 1.3, block.origin.z - 10.2), halfExtents: SIMD3<Float>(0.12, 0.90, 0.12), yaw: 0.0, color: transitColor))
            vertices.append(contentsOf: makeWorldBoxVertices(center: SIMD3<Float>(block.origin.x + 10.2, block.origin.y + 1.3, block.origin.z + 10.2), halfExtents: SIMD3<Float>(0.12, 0.90, 0.12), yaw: 0.0, color: transitColor))
        }

        if (block.tagMask & UInt32(MDTBBlockTagResidential)) != 0 {
            let porchColor = animatedColor(SIMD4<Float>(0.92, 0.80, 0.48, 1.0), intensity: 0.70 + energy * 0.20)
            vertices.append(contentsOf: makeWorldBoxVertices(center: SIMD3<Float>(block.origin.x + 38.2, block.origin.y + 2.0, block.origin.z + 33.8), halfExtents: SIMD3<Float>(0.86, 0.10, 0.10), yaw: 0.0, color: porchColor))
            vertices.append(contentsOf: makeWorldBoxVertices(center: SIMD3<Float>(block.origin.x - 41.6, block.origin.y + 3.0, block.origin.z + 15.0), halfExtents: SIMD3<Float>(1.2, 0.10, 0.10), yaw: 0.0, color: porchColor))
        }

        if (block.tagMask & UInt32(MDTBBlockTagCourt)) != 0 {
            let courtColor = animatedColor(baseColor, intensity: 0.66 + energy * 0.18)
            vertices.append(contentsOf: makeWorldBoxVertices(center: SIMD3<Float>(block.origin.x + 34.8, block.origin.y + 0.08, block.origin.z + 35.0), halfExtents: SIMD3<Float>(5.2, 0.03, 0.10), yaw: 0.0, color: courtColor))
        }

        if (profile.styleFlags & UInt32(MDTBPopulationStyleThroughTraffic)) != 0 {
            let laneColor = animatedColor(baseColor, intensity: 0.68 + energy * 0.14)
            vertices.append(contentsOf: makeWorldBoxVertices(center: SIMD3<Float>(block.origin.x, block.origin.y + 0.06, block.origin.z), halfExtents: SIMD3<Float>(0.10, 0.03, 6.4), yaw: .pi * 0.5, color: laneColor))
        }

        return vertices
    }

    private static func pedestrianSample(for point: SceneInterestPoint, pointIndex: Int, block: SceneBlock, profile: ScenePopulationProfile, blocks: [SceneBlock], roadLinks: [SceneRoadLink], interestPoints: [SceneInterestPoint], elapsedTime: Float) -> PedestrianSample? {
        let districtTempo = 0.76 + (profile.ambientEnergy * 0.46)
        let activityBoost: Float = (profile.styleFlags & UInt32(MDTBPopulationStyleRetailClustered)) != 0 ? 0.06 : 0.0
        let activeShare = min(0.88, 0.38 + (profile.pedestrianDensity * 0.42) + activityBoost)
        let cycleLength: Float = 8.5 + (Float(pointIndex % 3) * 1.35)
        let seed = (Float(pointIndex) * 1.37) + (Float(block.variant) * 0.61)
        let cycle = ((elapsedTime * districtTempo) + seed).truncatingRemainder(dividingBy: cycleLength)
        let activeWindow = cycleLength * activeShare
        guard cycle < activeWindow else {
            return nil
        }

        let progress = cycle / max(activeWindow, 0.01)
        let routePoints = pedestrianRoutePoints(for: point, pointIndex: pointIndex, block: block, profile: profile, blocks: blocks, roadLinks: roadLinks, interestPoints: interestPoints)
        let routeSample = sampleRoute(points: routePoints, progress: progress)
        let shoulder = SIMD3<Float>(cos(routeSample.heading), 0.0, sin(routeSample.heading))
        let sidewalkBias: Float = (pointIndex % 2 == 0) ? 0.78 : -0.78

        return PedestrianSample(
            position: SIMD3<Float>(routeSample.position.x + (shoulder.x * sidewalkBias), point.position.y, routeSample.position.z + (shoulder.z * sidewalkBias)),
            heading: routeSample.heading,
            tint: pedestrianTint(for: block)
        )
    }

    private static func vehicleSample(for point: SceneInterestPoint, pointIndex: Int, block: SceneBlock, profile: ScenePopulationProfile, blocks: [SceneBlock], roadLinks: [SceneRoadLink], elapsedTime: Float) -> VehicleSample? {
        let pace = 0.74 + (profile.vehicleDensity * 0.34)
        let trafficBoost: Float = (profile.styleFlags & UInt32(MDTBPopulationStyleThroughTraffic)) != 0 ? 0.08 : 0.0
        let activeShare = min(0.84, 0.26 + (profile.vehicleDensity * 0.40) + trafficBoost)
        let cycleLength: Float = 11.5 + Float(pointIndex % 2) * 2.4
        let seed = (Float(pointIndex) * 0.83) + (Float(block.variant) * 0.47)
        let cycle = ((elapsedTime * pace) + seed).truncatingRemainder(dividingBy: cycleLength)
        let activeWindow = cycleLength * activeShare
        guard cycle < activeWindow else {
            return nil
        }

        let progress = smoothstep(cycle / max(activeWindow, 0.01))
        let routePoints = vehicleRoutePoints(for: point, pointIndex: pointIndex, block: block, profile: profile, blocks: blocks, roadLinks: roadLinks)
        let routeSample = sampleRoute(points: routePoints, progress: progress)
        let vehicleKind = trafficVehicleKind(for: pointIndex, block: block, profile: profile)
        let laneMagnitude: Float = vehicleKind == UInt32(MDTBVehicleKindMoped) ? 1.28 : (vehicleKind == UInt32(MDTBVehicleKindCoupe) ? 1.60 : 1.72)
        let laneRight = SIMD3<Float>(cos(routeSample.heading), 0.0, sin(routeSample.heading))
        let laneOffset = laneRight * trafficLaneOffset(forHeading: routeSample.heading, laneMagnitude: laneMagnitude)

        return VehicleSample(
            position: SIMD3<Float>(routeSample.position.x + laneOffset.x, point.position.y, routeSample.position.z + laneOffset.z),
            yaw: routeSample.heading,
            tint: vehicleTint(for: block),
            kind: vehicleKind
        )
    }

    private static func pedestrianRoutePoints(for point: SceneInterestPoint, pointIndex: Int, block: SceneBlock, profile: ScenePopulationProfile, blocks: [SceneBlock], roadLinks: [SceneRoadLink], interestPoints: [SceneInterestPoint]) -> [SIMD3<Float>] {
        let blockIndex = blockIndex(for: block, blocks: blocks)
        let linkSequence = routeLinkSequence(for: blockIndex, pointIndex: pointIndex, block: block, profile: profile, roadLinks: roadLinks)
        guard let firstLinkIndex = linkSequence.first, firstLinkIndex < roadLinks.count else {
            return [point.position, block.origin]
        }

        let firstLink = roadLinks[firstLinkIndex]
        let neighborIndex = otherBlockIndex(for: firstLink, from: blockIndex)
        let firstDestination = hotspotDestination(for: neighborIndex, interestPoints: interestPoints, fallback: blocks[safe: neighborIndex]?.origin ?? block.origin)
        var points: [SIMD3<Float>] = [point.position, firstLink.midpoint, firstDestination]

        if linkSequence.count > 1, let neighborBlock = blocks[safe: neighborIndex] {
            let secondLink = roadLinks[linkSequence[1]]
            let secondNeighborIndex = otherBlockIndex(for: secondLink, from: neighborIndex)
            let secondDestination = hotspotDestination(for: secondNeighborIndex, interestPoints: interestPoints, fallback: blocks[safe: secondNeighborIndex]?.origin ?? neighborBlock.origin)
            points.append(secondLink.midpoint)
            points.append(secondDestination)
        }

        return points
    }

    private static func vehicleRoutePoints(for point: SceneInterestPoint, pointIndex: Int, block: SceneBlock, profile: ScenePopulationProfile, blocks: [SceneBlock], roadLinks: [SceneRoadLink]) -> [SIMD3<Float>] {
        let blockIndex = blockIndex(for: block, blocks: blocks)
        let linkSequence = routeLinkSequence(for: blockIndex, pointIndex: pointIndex, block: block, profile: profile, roadLinks: roadLinks)
        guard !linkSequence.isEmpty else {
            return [point.position, block.origin]
        }

        var points: [SIMD3<Float>] = [point.position]
        var currentBlockIndex = blockIndex

        for linkIndex in linkSequence where linkIndex < roadLinks.count {
            let link = roadLinks[linkIndex]
            let nextBlockIndex = otherBlockIndex(for: link, from: currentBlockIndex)
            points.append(link.midpoint)
            if let nextBlock = blocks[safe: nextBlockIndex] {
                points.append(nextBlock.origin)
            }
            currentBlockIndex = nextBlockIndex
        }

        return points
    }

    private static func routeLinkSequence(for blockIndex: Int, pointIndex: Int, block: SceneBlock, profile: ScenePopulationProfile, roadLinks: [SceneRoadLink]) -> [Int] {
        let primaryLinks = connectedLinkIndices(for: blockIndex, roadLinks: roadLinks)
        guard !primaryLinks.isEmpty else {
            return []
        }

        let primaryIndex = primaryLinks[(pointIndex + Int(block.variant)) % primaryLinks.count]
        var sequence: [Int] = [primaryIndex]
        let allowSecondHop = profile.travelBias >= 0.68 || (profile.styleFlags & UInt32(MDTBPopulationStyleThroughTraffic)) != 0

        guard allowSecondHop else {
            return sequence
        }

        let nextBlockIndex = otherBlockIndex(for: roadLinks[primaryIndex], from: blockIndex)
        let secondaryLinks = connectedLinkIndices(for: nextBlockIndex, roadLinks: roadLinks).filter { $0 != primaryIndex }
        if !secondaryLinks.isEmpty {
            let secondaryIndex = secondaryLinks[(pointIndex + Int(block.variant) + 1) % secondaryLinks.count]
            sequence.append(secondaryIndex)
        }

        return sequence
    }

    private static func connectedLinkIndices(for blockIndex: Int, roadLinks: [SceneRoadLink]) -> [Int] {
        roadLinks.enumerated().compactMap { index, link in
            let fromIndex = Int(link.fromBlockIndex)
            let toIndex = Int(link.toBlockIndex)
            return (fromIndex == blockIndex || toIndex == blockIndex) ? index : nil
        }
    }

    private static func otherBlockIndex(for link: SceneRoadLink, from blockIndex: Int) -> Int {
        let fromIndex = Int(link.fromBlockIndex)
        let toIndex = Int(link.toBlockIndex)
        return fromIndex == blockIndex ? toIndex : fromIndex
    }

    private static func hotspotDestination(for blockIndex: Int, interestPoints: [SceneInterestPoint], fallback: SIMD3<Float>) -> SIMD3<Float> {
        if let hotspot = interestPoints.first(where: { Int($0.blockIndex) == blockIndex && $0.kind == UInt32(MDTBInterestPointHotspot) }) {
            return hotspot.position
        }

        if let landmark = interestPoints.first(where: { Int($0.blockIndex) == blockIndex && $0.kind == UInt32(MDTBInterestPointLandmark) }) {
            return landmark.position
        }

        return fallback
    }

    private static func sampleRoute(points: [SIMD3<Float>], progress: Float) -> RouteSample {
        guard points.count >= 2 else {
            let fallback = points.first ?? SIMD3<Float>(repeating: 0)
            return RouteSample(position: fallback, heading: 0.0)
        }

        let clampedProgress = max(0.0, min(progress, 0.999))
        let segmentLengths = zip(points, points.dropFirst()).map { max(distanceSquared($0.0, $0.1).squareRoot(), 0.001) }
        let totalLength = max(segmentLengths.reduce(0, +), 0.001)
        let targetDistance = clampedProgress * totalLength
        var traversed: Float = 0.0

        for segmentIndex in 0 ..< segmentLengths.count {
            let segmentLength = segmentLengths[segmentIndex]
            if targetDistance <= traversed + segmentLength || segmentIndex == segmentLengths.count - 1 {
                let localT = max(0.0, min((targetDistance - traversed) / segmentLength, 1.0))
                let start = points[segmentIndex]
                let end = points[segmentIndex + 1]
                let position = start + ((end - start) * localT)
                let heading = atan2(end.x - start.x, -(end.z - start.z))
                return RouteSample(position: position, heading: heading)
            }

            traversed += segmentLength
        }

        let end = points[points.count - 1]
        let prior = points[points.count - 2]
        return RouteSample(position: end, heading: atan2(end.x - prior.x, -(end.z - prior.z)))
    }

    private static func blockIndex(for block: SceneBlock, blocks: [SceneBlock]) -> Int {
        blocks.firstIndex(where: { candidate in
            candidate.origin == block.origin &&
            candidate.kind == block.kind &&
            candidate.variant == block.variant
        }) ?? 0
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

    private static func trafficVehicleKind(for pointIndex: Int, block: SceneBlock, profile: ScenePopulationProfile) -> UInt32 {
        if block.frontageTemplate == UInt32(MDTBFrontageTemplateTransitMarket) {
            return pointIndex % 3 == 0 ? UInt32(MDTBVehicleKindMoped) : UInt32(MDTBVehicleKindCoupe)
        }

        if block.frontageTemplate == UInt32(MDTBFrontageTemplateResidentialCourt) {
            return pointIndex % 2 == 0 ? UInt32(MDTBVehicleKindCoupe) : UInt32(MDTBVehicleKindSedan)
        }

        if (profile.styleFlags & UInt32(MDTBPopulationStyleThroughTraffic)) != 0, pointIndex % 4 == 0 {
            return UInt32(MDTBVehicleKindCoupe)
        }

        return UInt32(MDTBVehicleKindSedan)
    }

    private static func trafficLaneOffset(forHeading heading: Float, laneMagnitude: Float) -> Float {
        if abs(cos(heading)) >= abs(sin(heading)) {
            return cos(heading) >= 0.0 ? laneMagnitude : -laneMagnitude
        }

        return sin(heading) >= 0.0 ? -laneMagnitude : laneMagnitude
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

    private static func makeVehiclePlaceholderVertices(position: SIMD3<Float>, yaw: Float, elapsedTime: Float, tint: SIMD4<Float>, kind: UInt32) -> [Vertex] {
        let lightPulse = 0.74 + (sin(elapsedTime * 3.6) * 0.14)
        let glassColor = SIMD4<Float>(0.70, 0.78, 0.84, 1.0)
        let trimColor = SIMD4<Float>(0.14, 0.16, 0.18, 1.0)
        let wheelColor = SIMD4<Float>(0.10, 0.11, 0.12, 1.0)

        switch kind {
        case UInt32(MDTBVehicleKindCoupe):
            return
                makeWorldBoxVertices(center: SIMD3<Float>(position.x, position.y + 0.32, position.z), halfExtents: SIMD3<Float>(1.02, 0.32, 1.84), yaw: yaw, color: tint) +
                makeWorldBoxVertices(center: SIMD3<Float>(position.x, position.y + 0.62, position.z - 0.12), halfExtents: SIMD3<Float>(0.58, 0.20, 0.92), yaw: yaw, color: animatedColor(tint, intensity: 0.80)) +
                makeWorldBoxVertices(center: SIMD3<Float>(position.x, position.y + 0.58, position.z + 0.78), halfExtents: SIMD3<Float>(0.52, 0.14, 0.08), yaw: yaw, color: glassColor) +
                makeWorldBoxVertices(center: SIMD3<Float>(position.x - 0.40, position.y + 0.24, position.z + 1.68), halfExtents: SIMD3<Float>(0.10, 0.10, 0.12), yaw: yaw, color: wheelColor) +
                makeWorldBoxVertices(center: SIMD3<Float>(position.x + 0.40, position.y + 0.24, position.z + 1.68), halfExtents: SIMD3<Float>(0.10, 0.10, 0.12), yaw: yaw, color: wheelColor) +
                makeWorldBoxVertices(center: SIMD3<Float>(position.x - 0.40, position.y + 0.24, position.z - 1.48), halfExtents: SIMD3<Float>(0.10, 0.10, 0.12), yaw: yaw, color: wheelColor) +
                makeWorldBoxVertices(center: SIMD3<Float>(position.x + 0.40, position.y + 0.24, position.z - 1.48), halfExtents: SIMD3<Float>(0.10, 0.10, 0.12), yaw: yaw, color: wheelColor) +
                makeWorldBoxVertices(center: SIMD3<Float>(position.x - 0.36, position.y + 0.42, position.z + 1.76), halfExtents: SIMD3<Float>(0.10, 0.06, 0.04), yaw: yaw, color: SIMD4<Float>(lightPulse, lightPulse, 0.88, 1.0)) +
                makeWorldBoxVertices(center: SIMD3<Float>(position.x + 0.36, position.y + 0.42, position.z + 1.76), halfExtents: SIMD3<Float>(0.10, 0.06, 0.04), yaw: yaw, color: SIMD4<Float>(lightPulse, lightPulse, 0.88, 1.0))
        case UInt32(MDTBVehicleKindMoped):
            return
                makeWorldBoxVertices(center: SIMD3<Float>(position.x, position.y + 0.26, position.z + 0.18), halfExtents: SIMD3<Float>(0.18, 0.20, 0.84), yaw: yaw, color: tint) +
                makeWorldBoxVertices(center: SIMD3<Float>(position.x, position.y + 0.60, position.z - 0.06), halfExtents: SIMD3<Float>(0.10, 0.10, 0.36), yaw: yaw, color: animatedColor(tint, intensity: 0.82)) +
                makeWorldBoxVertices(center: SIMD3<Float>(position.x, position.y + 0.82, position.z - 0.48), halfExtents: SIMD3<Float>(0.42, 0.04, 0.04), yaw: yaw, color: trimColor) +
                makeWorldBoxVertices(center: SIMD3<Float>(position.x, position.y + 0.76, position.z + 0.10), halfExtents: SIMD3<Float>(0.20, 0.04, 0.20), yaw: yaw, color: trimColor) +
                makeWorldBoxVertices(center: SIMD3<Float>(position.x, position.y + 0.34, position.z + 1.02), halfExtents: SIMD3<Float>(0.14, 0.14, 0.14), yaw: yaw, color: wheelColor) +
                makeWorldBoxVertices(center: SIMD3<Float>(position.x, position.y + 0.34, position.z - 1.00), halfExtents: SIMD3<Float>(0.14, 0.14, 0.14), yaw: yaw, color: wheelColor) +
                makeWorldBoxVertices(center: SIMD3<Float>(position.x, position.y + 0.54, position.z + 1.18), halfExtents: SIMD3<Float>(0.08, 0.08, 0.03), yaw: yaw, color: SIMD4<Float>(lightPulse, lightPulse, 0.88, 1.0))
        default:
            return
                makeWorldBoxVertices(center: SIMD3<Float>(position.x, position.y + 0.36, position.z), halfExtents: SIMD3<Float>(1.18, 0.36, 2.08), yaw: yaw, color: tint) +
                makeWorldBoxVertices(center: SIMD3<Float>(position.x, position.y + 0.74, position.z - 0.08), halfExtents: SIMD3<Float>(0.72, 0.26, 1.08), yaw: yaw, color: animatedColor(tint, intensity: 0.82)) +
                makeWorldBoxVertices(center: SIMD3<Float>(position.x, position.y + 0.68, position.z + 0.88), halfExtents: SIMD3<Float>(0.62, 0.18, 0.08), yaw: yaw, color: glassColor) +
                makeWorldBoxVertices(center: SIMD3<Float>(position.x - 0.46, position.y + 0.44, position.z + 1.98), halfExtents: SIMD3<Float>(0.12, 0.08, 0.04), yaw: yaw, color: SIMD4<Float>(lightPulse, lightPulse, 0.88, 1.0)) +
                makeWorldBoxVertices(center: SIMD3<Float>(position.x + 0.46, position.y + 0.44, position.z + 1.98), halfExtents: SIMD3<Float>(0.12, 0.08, 0.04), yaw: yaw, color: SIMD4<Float>(lightPulse, lightPulse, 0.88, 1.0))
        }
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

    private static func populationProfile(for blockIndex: Int, block: SceneBlock, populationProfiles: [ScenePopulationProfile]) -> ScenePopulationProfile {
        if blockIndex < populationProfiles.count, populationProfiles[blockIndex].blockIndex == UInt32(blockIndex) {
            return populationProfiles[blockIndex]
        }

        if let matchedProfile = populationProfiles.first(where: { $0.blockIndex == UInt32(blockIndex) }) {
            return matchedProfile
        }

        var styleFlags: UInt32 = 0
        if (block.tagMask & UInt32(MDTBBlockTagTransit)) != 0 {
            styleFlags |= UInt32(MDTBPopulationStyleTransitHeavy)
        }
        if (block.tagMask & UInt32(MDTBBlockTagRetail)) != 0 {
            styleFlags |= UInt32(MDTBPopulationStyleRetailClustered)
        }
        if (block.tagMask & UInt32(MDTBBlockTagResidential)) != 0 {
            styleFlags |= UInt32(MDTBPopulationStyleResidentialCalm)
        }
        if (block.tagMask & UInt32(MDTBBlockTagSpur)) != 0 {
            styleFlags |= UInt32(MDTBPopulationStyleThroughTraffic)
        }

        return ScenePopulationProfile(
            blockIndex: UInt32(blockIndex),
            pedestrianDensity: block.district == UInt32(MDTBDistrictMapleHeights) ? 0.56 : 0.72,
            vehicleDensity: block.district == UInt32(MDTBDistrictMapleHeights) ? 0.38 : 0.64,
            ambientEnergy: block.district == UInt32(MDTBDistrictMapleHeights) ? 0.48 : 0.76,
            travelBias: (block.tagMask & UInt32(MDTBBlockTagSpur)) != 0 ? 0.82 : 0.56,
            styleFlags: styleFlags
        )
    }

    private static func districtAmbientColor(for block: SceneBlock) -> SIMD4<Float> {
        switch block.district {
        case UInt32(MDTBDistrictMapleHeights):
            return SIMD4<Float>(0.50, 0.68, 0.40, 1.0)
        case UInt32(MDTBDistrictMarketSpur):
            return SIMD4<Float>(0.28, 0.66, 0.84, 1.0)
        default:
            return SIMD4<Float>(0.88, 0.56, 0.24, 1.0)
        }
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

    private static func activeChunkIndices(activeBlockIndices: Set<Int>, blocks: [SceneBlock]) -> Set<Int> {
        Set(
            activeBlockIndices.compactMap { blockIndex in
                guard let block = blocks[safe: blockIndex] else {
                    return nil
                }
                return Int(block.chunkIndex)
            }
        )
    }

    private static func visibleBlockIndices(activeChunkIndices: Set<Int>, blocks: [SceneBlock]) -> Set<Int> {
        Set(
            blocks.enumerated().compactMap { index, block in
                activeChunkIndices.contains(Int(block.chunkIndex)) ? index : nil
            }
        )
    }

    private static func chunkCount(_ blocks: [SceneBlock]) -> Int {
        Set(blocks.map { Int($0.chunkIndex) }).count
    }

    private static func activitySummary(state: MDTBEngineState, blocks: [SceneBlock], roadLinks: [SceneRoadLink], interestPoints: [SceneInterestPoint], populationProfiles: [ScenePopulationProfile], vehicleAnchors: [SceneVehicleAnchor]) -> String {
        let blockLabel = blockName(for: state.active_block_index, blocks: blocks)
        let linkLabel = linkSummary(for: state.active_link_index, roadLinks: roadLinks, blocks: blocks)
        let population = populationActivity(state: state, blocks: blocks, roadLinks: roadLinks, interestPoints: interestPoints, populationProfiles: populationProfiles)
        let chunkLabelValue = activeChunkLabel(state: state, blocks: blocks)
        let visibleBlocks = visibleBlockCount(state: state, blocks: blocks, roadLinks: roadLinks)
        let hotspots = hotspotCount(state: state, blocks: blocks, roadLinks: roadLinks, interestPoints: interestPoints)
        let stagedVehicles = stagedVehicleCount(state: state, blocks: blocks, roadLinks: roadLinks, vehicleAnchors: vehicleAnchors)
        return "\(blockLabel) / \(chunkLabelValue) / \(visibleBlocks) visible / \(hotspots) hot / \(stagedVehicles) staged / \(population.livePedestrians) live ped / \(population.liveVehicles) live veh / \(linkLabel)"
    }

    private static func blockSummary(state: MDTBEngineState, blocks: [SceneBlock], populationProfiles: [ScenePopulationProfile]) -> String {
        guard state.active_block_index < blocks.count else {
            return "none"
        }

        let blockIndex = Int(state.active_block_index)
        let block = blocks[blockIndex]
        let profile = populationProfile(for: blockIndex, block: block, populationProfiles: populationProfiles)
        return "\(blockKindLabel(block.kind)) / \(districtLabel(block.district)) / \(tagLabel(block.tagMask)) / \(frontageTemplateLabel(block.frontageTemplate)) / \(chunkLabel(block.chunkIndex)) / \(profileSummary(profile)) / r \(formatScalar(block.activationRadius))"
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

    private static func populationActivity(state: MDTBEngineState, blocks: [SceneBlock], roadLinks: [SceneRoadLink], interestPoints: [SceneInterestPoint], populationProfiles: [ScenePopulationProfile]) -> PopulationActivity {
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
        let activeChunks = activeChunkIndices(activeBlockIndices: activeBlockIndices, blocks: blocks)
        let visibleBlocks = visibleBlockIndices(activeChunkIndices: activeChunks, blocks: blocks)

        var pedestrians = 0
        var vehicles = 0

        for (pointIndex, point) in interestPoints.enumerated() {
            let blockIndex = Int(point.blockIndex)
            guard blockIndex < blocks.count, visibleBlocks.contains(blockIndex) else {
                continue
            }

            let block = blocks[blockIndex]
            let profile = populationProfile(for: blockIndex, block: block, populationProfiles: populationProfiles)
            switch point.kind {
            case UInt32(MDTBInterestPointPedestrianSpawn):
                if pedestrianSample(for: point, pointIndex: pointIndex, block: block, profile: profile, blocks: blocks, roadLinks: roadLinks, interestPoints: interestPoints, elapsedTime: state.elapsed_time) != nil {
                    pedestrians += 1
                }
            case UInt32(MDTBInterestPointVehicleSpawn):
                if vehicleSample(for: point, pointIndex: pointIndex, block: block, profile: profile, blocks: blocks, roadLinks: roadLinks, elapsedTime: state.elapsed_time) != nil {
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

    private static func profileSummary(_ profile: ScenePopulationProfile) -> String {
        "pop \(formatScalar(profile.pedestrianDensity))/\(formatScalar(profile.vehicleDensity))/\(formatScalar(profile.ambientEnergy)) \(styleLabel(profile.styleFlags))"
    }

    private static func activeChunkLabel(state: MDTBEngineState, blocks: [SceneBlock]) -> String {
        guard state.active_block_index < blocks.count else {
            return "no-chunk"
        }

        return chunkLabel(blocks[Int(state.active_block_index)].chunkIndex)
    }

    private static func visibleBlockCount(state: MDTBEngineState, blocks: [SceneBlock], roadLinks: [SceneRoadLink]) -> Int {
        let actorPosition = SIMD3<Float>(state.actor_position.x, state.actor_position.y, state.actor_position.z)
        let activeBlocks = activeBlockIndices(for: actorPosition, activeBlockIndex: state.active_block_index, activeLinkIndex: state.active_link_index, blocks: blocks, roadLinks: roadLinks)
        let activeChunks = activeChunkIndices(activeBlockIndices: activeBlocks, blocks: blocks)
        return visibleBlockIndices(activeChunkIndices: activeChunks, blocks: blocks).count
    }

    private static func hotspotCount(state: MDTBEngineState, blocks: [SceneBlock], roadLinks: [SceneRoadLink], interestPoints: [SceneInterestPoint]) -> Int {
        let actorPosition = SIMD3<Float>(state.actor_position.x, state.actor_position.y, state.actor_position.z)
        let activeBlocks = activeBlockIndices(for: actorPosition, activeBlockIndex: state.active_block_index, activeLinkIndex: state.active_link_index, blocks: blocks, roadLinks: roadLinks)
        let activeChunks = activeChunkIndices(activeBlockIndices: activeBlocks, blocks: blocks)
        let visibleBlocks = visibleBlockIndices(activeChunkIndices: activeChunks, blocks: blocks)
        return interestPoints.reduce(into: 0) { count, point in
            if point.kind == UInt32(MDTBInterestPointHotspot), visibleBlocks.contains(Int(point.blockIndex)) {
                count += 1
            }
        }
    }

    private static func stagedVehicleCount(state: MDTBEngineState, blocks: [SceneBlock], roadLinks: [SceneRoadLink], vehicleAnchors: [SceneVehicleAnchor]) -> Int {
        let actorPosition = SIMD3<Float>(state.actor_position.x, state.actor_position.y, state.actor_position.z)
        let activeBlocks = activeBlockIndices(for: actorPosition, activeBlockIndex: state.active_block_index, activeLinkIndex: state.active_link_index, blocks: blocks, roadLinks: roadLinks)
        let activeChunks = activeChunkIndices(activeBlockIndices: activeBlocks, blocks: blocks)
        let visibleBlocks = visibleBlockIndices(activeChunkIndices: activeChunks, blocks: blocks)
        return vehicleAnchors.reduce(into: 0) { count, anchor in
            if visibleBlocks.contains(Int(anchor.blockIndex)) {
                count += 1
            }
        }
    }

    private static func vehicleStatusSummary(state: MDTBEngineState, vehicleAnchors: [SceneVehicleAnchor], blocks: [SceneBlock]) -> String {
        if state.traversal_mode == UInt32(MDTBTraversalModeVehicle) {
            let vehicleLabel = vehicleKindLabel(state.active_vehicle_kind)
            let blockLabel = blockName(for: state.active_block_index, blocks: blocks)
            let surfaceGrip = formatScalar(state.active_vehicle_surface_grip)
            let laneError = formatScalar(state.active_vehicle_lane_error)
            let bump = formatScalar(state.active_vehicle_collision_pulse)
            return "\(vehicleLabel) active / \(blockLabel) / \(formatScalar(state.active_vehicle_speed)) m/s / grip \(surfaceGrip) / lane \(laneError) / bump \(bump)"
        }

        guard state.nearby_vehicle_anchor_index < vehicleAnchors.count else {
            return "on-foot / no staged vehicle nearby"
        }

        let anchor = vehicleAnchors[Int(state.nearby_vehicle_anchor_index)]
        let distance = sqrt(distanceSquared(
            SIMD3<Float>(state.actor_position.x, state.actor_position.y, state.actor_position.z),
            anchor.position
        ))
        return "\(vehicleKindLabel(anchor.kind)) nearby / \(formatScalar(distance))m / \(parkingStateLabel(anchor.parkingState)) / \(blockName(for: anchor.blockIndex, blocks: blocks))"
    }

    private static func interactionSummary(state: MDTBEngineState, vehicleAnchors: [SceneVehicleAnchor], blocks: [SceneBlock]) -> String {
        if state.traversal_mode == UInt32(MDTBTraversalModeVehicle) {
            if abs(state.active_vehicle_speed) <= 1.4 {
                return "press F to exit \(vehicleKindLabel(state.active_vehicle_kind))"
            }
            return "slow down to exit \(vehicleKindLabel(state.active_vehicle_kind))"
        }

        guard state.nearby_vehicle_anchor_index < vehicleAnchors.count else {
            return "walk up to a staged vehicle and press F"
        }

        let anchor = vehicleAnchors[Int(state.nearby_vehicle_anchor_index)]
        let action = anchor.kind == UInt32(MDTBVehicleKindMoped) ? "ride" : "drive"
        return "press F to \(action) \(vehicleKindLabel(anchor.kind)) at \(parkingStateLabel(anchor.parkingState)) \(blockName(for: anchor.blockIndex, blocks: blocks))"
    }

    private static func styleLabel(_ value: UInt32) -> String {
        var labels: [String] = []

        if (value & UInt32(MDTBPopulationStyleTransitHeavy)) != 0 {
            labels.append("transit")
        }
        if (value & UInt32(MDTBPopulationStyleResidentialCalm)) != 0 {
            labels.append("calm")
        }
        if (value & UInt32(MDTBPopulationStyleRetailClustered)) != 0 {
            labels.append("retail")
        }
        if (value & UInt32(MDTBPopulationStyleThroughTraffic)) != 0 {
            labels.append("through")
        }

        return labels.isEmpty ? "base" : labels.joined(separator: "+")
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

    private static func frontageTemplateLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBFrontageTemplateResidentialCourt):
            return "res-court"
        case UInt32(MDTBFrontageTemplateTransitMarket):
            return "transit-market"
        case UInt32(MDTBFrontageTemplateServiceSpur):
            return "service-spur"
        default:
            return "civic-retail"
        }
    }

    private static func chunkLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBWorldChunkEastGrid):
            return "east-grid"
        default:
            return "west-grid"
        }
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
        case UInt32(MDTBInterestPointHotspot):
            return "hotspot"
        default:
            return "hook"
        }
    }

    private static func vehicleKindLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBVehicleKindCoupe):
            return "coupe"
        case UInt32(MDTBVehicleKindMoped):
            return "moped"
        case UInt32(MDTBVehicleKindSedan):
            return "sedan"
        default:
            return "vehicle"
        }
    }

    private static func parkingStateLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBVehicleParkingStateService):
            return "service-spot"
        default:
            return "curbside"
        }
    }

    private static func cameraModeLabel(_ value: UInt32, traversalMode: UInt32) -> String {
        let cameraMode = value == UInt32(MDTBCameraModeThirdPerson) ? "third-person" : "first-person"
        let traversalLabel = traversalMode == UInt32(MDTBTraversalModeVehicle) ? "vehicle" : "on-foot"
        return "\(cameraMode) \(traversalLabel)"
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

private extension Array {
    subscript(safe index: Int) -> Element? {
        guard indices.contains(index) else {
            return nil
        }

        return self[index]
    }
}
