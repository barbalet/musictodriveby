import Metal
import MetalKit
import QuartzCore
import Foundation
import simd

#if SWIFT_PACKAGE
import EngineCore
#endif

struct Vertex {
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
    var roadClass: UInt32
    var corridor: UInt32
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

private struct SceneTrafficOccupancy {
    var position: SIMD3<Float>
    var radius: Float
    var blockIndex: UInt32
    var axis: UInt32
    var reason: UInt32
    var strength: Float
}

private struct PopulationActivity {
    var livePedestrians: Int
    var reactingPedestrians: Int
    var liveVehicles: Int
    var yieldingVehicles: Int
}

private struct PedestrianSample {
    var position: SIMD3<Float>
    var heading: Float
    var tint: SIMD4<Float>
    var reactionIntensity: Float
}

private struct VehicleSample {
    var position: SIMD3<Float>
    var yaw: Float
    var tint: SIMD4<Float>
    var kind: UInt32
    var yieldIntensity: Float
    var hazardIntensity: Float
}

private struct RouteSample {
    var position: SIMD3<Float>
    var heading: Float
}

private struct AmbientFrame {
    var vertices: [Vertex]
    var trafficHazard: Float
}

@MainActor
final class Renderer: NSObject, MTKViewDelegate {
    private static let dynamicVertexBudget = 4096 * 36
    private static let vertexGrowthStep = 256 * 36
    private static let vehicleMountRadius: Float = 3.6
    private static let artMeshes = ArtMeshCatalog.shared

    private let device: MTLDevice
    private let commandQueue: MTLCommandQueue
    private let pipelineState: MTLRenderPipelineState
    private let depthState: MTLDepthStencilState
    private var vertexBuffer: MTLBuffer
    private var maxVertexCount: Int
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

    private func ensureVertexCapacity(requiredCount: Int) -> Bool {
        guard requiredCount > maxVertexCount else {
            return true
        }

        let growthStep = max(Self.vertexGrowthStep, requiredCount - maxVertexCount)
        let newCapacity = requiredCount + growthStep
        guard let replacementBuffer = device.makeBuffer(length: MemoryLayout<Vertex>.stride * newCapacity, options: .storageModeShared) else {
            assertionFailure("Unable to grow vertex buffer for frame requiring \(requiredCount) vertices.")
            return false
        }

        replacementBuffer.label = "Graybox Scene"
        vertexBuffer = replacementBuffer
        maxVertexCount = newCapacity
        return true
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
        let trafficOccupancies = Self.loadTrafficOccupancies()

        fpsWindowTime += Double(deltaTime)
        fpsWindowFrames += 1

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
            visibleBlockIndices: visibleBlockIndices
        )
        let environmentArtVertices = Self.makeEnvironmentArtVertices(
            visibleBlockIndices: visibleBlockIndices,
            blocks: blocks
        )
        let ambientFrame = Self.makeAmbientVertices(
            elapsedTime: engineState.elapsed_time,
            activeLinkIndex: engineState.active_link_index,
            visibleBlockIndices: visibleBlockIndices,
            state: engineState,
            trafficOccupancies: trafficOccupancies,
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
        let combatVertices = Self.makeCombatVertices(
            state: engineState,
            elapsedTime: engineState.elapsed_time
        )
        let actorVertices = isThirdPerson && engineState.traversal_mode == UInt32(MDTBTraversalModeOnFoot)
            ? Self.makeActorVertices(
                position: actorPosition,
                heading: engineState.actor_heading,
                cameraYaw: engineState.camera.yaw,
                speed: engineState.camera.move_speed,
                elapsedTime: engineState.elapsed_time,
                equippedWeapon: engineState.equipped_weapon_kind,
                meleeAttackPhase: engineState.melee_attack_phase,
                meleeAttackTimer: engineState.melee_attack_timer,
                firearmReloading: engineState.firearm_reloading != 0,
                firearmReloadTimer: engineState.firearm_reload_timer,
                firearmShotTimer: engineState.firearm_last_shot_timer
            )
            : []
        let drawVertices = staticVertices + environmentArtVertices + ambientFrame.vertices + combatVertices + vehicleAnchorVertices + actorVertices
        let drawVertexCount = drawVertices.count
        latestVisibleStaticBoxCount = staticVertices.count / 36

        if fpsWindowTime >= 0.25 {
            latestFPS = Double(fpsWindowFrames) / fpsWindowTime
            fpsWindowTime = 0
            fpsWindowFrames = 0
            publishDebugState(vehicleAnchors: vehicleAnchors, trafficOccupancies: trafficOccupancies, trafficHazard: ambientFrame.trafficHazard)
        }

        guard ensureVertexCapacity(requiredCount: drawVertexCount) else {
            return
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
            farZ: 520
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

    private func publishDebugState(vehicleAnchors: [SceneVehicleAnchor], trafficOccupancies: [SceneTrafficOccupancy], trafficHazard: Float) {
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
        let snapshotLayout = "\(blocks.count) blocks / \(Self.districtCount(blocks)) districts / \(Self.chunkCount(blocks)) tiles / \(Self.corridorCount(roadLinks)) corridors / \(roadLinks.count) links / \(vehicleAnchors.count) staged veh / \(trafficOccupancies.count) occ / \(interestPoints.count) hooks / \(dynamicProps.count) dynamic / \(latestVisibleStaticBoxCount)/\(staticSceneBoxes.count) static"
        let snapshotActivity = Self.activitySummary(state: engineState, blocks: blocks, roadLinks: roadLinks, interestPoints: interestPoints, populationProfiles: populationProfiles, trafficOccupancies: trafficOccupancies, vehicleAnchors: vehicleAnchors)
        let snapshotVehicle = Self.vehicleStatusSummary(state: engineState, vehicleAnchors: vehicleAnchors, blocks: blocks)
        let snapshotCombat = Self.combatSummary(state: engineState)
        let snapshotInteraction = Self.interactionSummary(state: engineState, vehicleAnchors: vehicleAnchors, blocks: blocks)
        let snapshotSelection = Self.handoffSelectionSummary(state: engineState, vehicleAnchors: vehicleAnchors, blocks: blocks)
        let snapshotHazard = Self.trafficHazardSummary(state: engineState, trafficHazard: trafficHazard)
        let snapshotBlock = Self.blockSummary(state: engineState, blocks: blocks, populationProfiles: populationProfiles)
        let snapshotNearestHook = Self.nearestInterestPointSummary(for: actorPosition, interestPoints: interestPoints, blocks: blocks)
        let snapshotHUD = Self.combatHUDModel(state: engineState, interactionSummary: snapshotInteraction, trafficHazard: trafficHazard, blocks: blocks, roadLinks: roadLinks)

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
            combatSummary: snapshotCombat,
            interactionSummary: snapshotInteraction,
            selectionSummary: snapshotSelection,
            hazardSummary: snapshotHazard,
            currentBlock: snapshotBlock,
            nearestHook: snapshotNearestHook,
            combatHUD: snapshotHUD
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
                axis: link.axis,
                roadClass: link.road_class,
                corridor: link.corridor
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

    private static func loadTrafficOccupancies() -> [SceneTrafficOccupancy] {
        let occupancyCount = Int(mdtb_engine_traffic_occupancy_count())
        guard occupancyCount > 0 else {
            return []
        }

        let occupancyBuffer = UnsafeMutablePointer<MDTBTrafficOccupancy>.allocate(capacity: occupancyCount)
        defer {
            occupancyBuffer.deallocate()
        }

        mdtb_engine_copy_traffic_occupancies(occupancyBuffer, occupancyCount)

        return (0 ..< occupancyCount).map { index in
            let occupancy = occupancyBuffer[index]
            return SceneTrafficOccupancy(
                position: SIMD3<Float>(occupancy.position.x, occupancy.position.y, occupancy.position.z),
                radius: occupancy.radius,
                blockIndex: occupancy.block_index,
                axis: occupancy.axis,
                reason: occupancy.reason,
                strength: occupancy.strength
            )
        }
    }

    private static func makeStaticSceneVertices(staticSceneBoxes: [SceneStaticBox], visibleBlockIndices: Set<Int>) -> [Vertex] {
        var vertices: [Vertex] = []
        vertices.reserveCapacity(staticSceneBoxes.count * 36)

        for sceneBox in staticSceneBoxes {
            if sceneBox.layer == UInt32(MDTBSceneLayerShared) {
                vertices.append(contentsOf: makeBoxVertices(sceneBox.box))
                continue
            }

            if sceneBox.layer == UInt32(MDTBSceneLayerBlockOwned) && visibleBlockIndices.contains(Int(sceneBox.blockIndex)) {
                continue
            }

            if sceneBox.layer == UInt32(MDTBSceneLayerBlockOwned) {
                vertices.append(contentsOf: makeBoxVertices(sceneBox.box))
            }
        }

        return vertices
    }

    private static func makeEnvironmentArtVertices(visibleBlockIndices: Set<Int>, blocks: [SceneBlock]) -> [Vertex] {
        var vertices: [Vertex] = []

        for blockIndex in visibleBlockIndices.sorted() where blockIndex < blocks.count {
            let block = blocks[blockIndex]
            let origin = block.origin
            let blockTint = environmentTint(for: block)

            if block.frontageTemplate == UInt32(MDTBFrontageTemplateResidentialCourt) || block.kind == UInt32(MDTBBlockKindResidential) {
                let primaryScale = artMeshes.fittedUniformScale(for: .suburbanHouseA, targetWidth: 18.0, targetDepth: 16.0, targetHeight: 18.0)
                let secondaryScale = artMeshes.fittedUniformScale(for: .suburbanHouseL, targetWidth: 16.0, targetDepth: 16.0, targetHeight: 17.0)
                let treeScale = artMeshes.fittedUniformScale(for: .suburbanTree, targetWidth: 5.0, targetDepth: 5.0, targetHeight: 9.0)
                let fenceScale = artMeshes.fittedUniformScale(for: .suburbanFence, targetWidth: 14.0, targetDepth: 1.4, targetHeight: 2.5)
                let planterScale = artMeshes.fittedUniformScale(for: .suburbanPlanter, targetWidth: 2.4, targetDepth: 2.4, targetHeight: 1.8)
                let lightScale = artMeshes.fittedUniformScale(for: .lightSquare, targetWidth: 2.2, targetDepth: 2.2, targetHeight: 8.0)

                vertices.append(contentsOf: artMeshes.makeVertices(for: .suburbanHouseA, position: origin + SIMD3<Float>(-18.0, 0.0, 12.0), yaw: 0.0, scale: primaryScale, tint: blockTint))
                vertices.append(contentsOf: artMeshes.makeVertices(for: .suburbanHouseL, position: origin + SIMD3<Float>(18.0, 0.0, -12.0), yaw: .pi, scale: secondaryScale, tint: blockTint))
                vertices.append(contentsOf: artMeshes.makeVertices(for: .suburbanTree, position: origin + SIMD3<Float>(-32.0, 0.0, -26.0), yaw: 0.0, scale: treeScale, tint: SIMD4<Float>(0.92, 1.0, 0.92, 1.0)))
                vertices.append(contentsOf: artMeshes.makeVertices(for: .suburbanFence, position: origin + SIMD3<Float>(0.0, 0.0, 27.0), yaw: 0.0, scale: fenceScale, tint: SIMD4<Float>(1.0, 1.0, 1.0, 1.0)))
                vertices.append(contentsOf: artMeshes.makeVertices(for: .suburbanPlanter, position: origin + SIMD3<Float>(-10.0, 0.0, 25.0), yaw: 0.0, scale: planterScale, tint: SIMD4<Float>(1.0, 1.0, 1.0, 1.0)))
                vertices.append(contentsOf: artMeshes.makeVertices(for: .lightSquare, position: origin + SIMD3<Float>(36.0, 0.0, 36.0), yaw: 0.0, scale: lightScale))
                continue
            }

            if block.frontageTemplate == UInt32(MDTBFrontageTemplateServiceSpur) || (block.tagMask & UInt32(MDTBBlockTagSpur)) != 0 {
                let primaryScale = artMeshes.fittedUniformScale(for: .industrialBuildingD, targetWidth: 24.0, targetDepth: 20.0, targetHeight: 18.0)
                let secondaryScale = artMeshes.fittedUniformScale(for: .industrialBuildingN, targetWidth: 18.0, targetDepth: 16.0, targetHeight: 14.0)
                let tankScale = artMeshes.fittedUniformScale(for: .industrialTank, targetWidth: 5.0, targetDepth: 5.0, targetHeight: 4.0)
                let chimneyScale = artMeshes.fittedUniformScale(for: .industrialChimney, targetWidth: 3.0, targetDepth: 3.0, targetHeight: 10.0)
                let coneScale = artMeshes.fittedUniformScale(for: .constructionCone, targetWidth: 1.0, targetDepth: 1.0, targetHeight: 1.4)
                let lightScale = artMeshes.fittedUniformScale(for: .lightSquare, targetWidth: 2.2, targetDepth: 2.2, targetHeight: 8.0)

                vertices.append(contentsOf: artMeshes.makeVertices(for: .industrialBuildingD, position: origin + SIMD3<Float>(-12.0, 0.0, 4.0), yaw: 0.0, scale: primaryScale, tint: blockTint))
                vertices.append(contentsOf: artMeshes.makeVertices(for: .industrialBuildingN, position: origin + SIMD3<Float>(18.0, 0.0, -18.0), yaw: .pi * 0.5, scale: secondaryScale, tint: blockTint))
                vertices.append(contentsOf: artMeshes.makeVertices(for: .industrialTank, position: origin + SIMD3<Float>(26.0, 0.0, 18.0), yaw: 0.0, scale: tankScale))
                vertices.append(contentsOf: artMeshes.makeVertices(for: .industrialChimney, position: origin + SIMD3<Float>(-22.0, 0.0, -20.0), yaw: 0.0, scale: chimneyScale))
                vertices.append(contentsOf: artMeshes.makeVertices(for: .constructionCone, position: origin + SIMD3<Float>(30.0, 0.0, 30.0), yaw: 0.0, scale: coneScale))
                vertices.append(contentsOf: artMeshes.makeVertices(for: .lightSquare, position: origin + SIMD3<Float>(36.0, 0.0, -34.0), yaw: 0.0, scale: lightScale))
                continue
            }

            let downtownBlock =
                block.kind == UInt32(MDTBBlockKindHub) ||
                block.district == UInt32(MDTBDistrictKoreatown) ||
                block.district == UInt32(MDTBDistrictUniversityPark) ||
                block.district == UInt32(MDTBDistrictSouthPark)

            let primaryMesh: ArtMeshID = downtownBlock ? .commercialTowerA : .commercialLowA
            let secondaryMesh: ArtMeshID = downtownBlock ? .commercialWideA : .commercialAwning
            let primaryScale = artMeshes.fittedUniformScale(for: primaryMesh, targetWidth: downtownBlock ? 18.0 : 22.0, targetDepth: downtownBlock ? 18.0 : 22.0, targetHeight: downtownBlock ? 34.0 : 18.0)
            let secondaryScale = artMeshes.fittedUniformScale(for: secondaryMesh, targetWidth: downtownBlock ? 26.0 : 10.0, targetDepth: downtownBlock ? 20.0 : 6.0, targetHeight: downtownBlock ? 16.0 : 4.0)
            let awningScale = artMeshes.fittedUniformScale(for: .commercialAwning, targetWidth: 8.0, targetDepth: 3.0, targetHeight: 3.0)
            let signScale = artMeshes.fittedUniformScale(for: .signHighway, targetWidth: 5.0, targetDepth: 1.6, targetHeight: 5.0)
            let lightScale = artMeshes.fittedUniformScale(for: .lightSquare, targetWidth: 2.2, targetDepth: 2.2, targetHeight: 8.0)

            vertices.append(contentsOf: artMeshes.makeVertices(for: primaryMesh, position: origin + SIMD3<Float>(-10.0, 0.0, 0.0), yaw: 0.0, scale: primaryScale, tint: blockTint))
            vertices.append(contentsOf: artMeshes.makeVertices(for: secondaryMesh, position: origin + SIMD3<Float>(18.0, 0.0, -12.0), yaw: .pi * 0.5, scale: secondaryScale, tint: blockTint))
            vertices.append(contentsOf: artMeshes.makeVertices(for: .commercialAwning, position: origin + SIMD3<Float>(12.0, 0.0, 18.0), yaw: 0.0, scale: awningScale))
            vertices.append(contentsOf: artMeshes.makeVertices(for: .signHighway, position: origin + SIMD3<Float>(-34.0, 0.0, 34.0), yaw: 0.0, scale: signScale))
            vertices.append(contentsOf: artMeshes.makeVertices(for: .lightSquare, position: origin + SIMD3<Float>(34.0, 0.0, -34.0), yaw: 0.0, scale: lightScale))
        }

        return vertices
    }

    private static func makeAmbientVertices(elapsedTime: Float, activeLinkIndex: UInt32, visibleBlockIndices: Set<Int>, state: MDTBEngineState, trafficOccupancies: [SceneTrafficOccupancy], blocks: [SceneBlock], roadLinks: [SceneRoadLink], interestPoints: [SceneInterestPoint], dynamicProps: [SceneDynamicProp], populationProfiles: [ScenePopulationProfile]) -> AmbientFrame {
        var vertices: [Vertex] = []
        var trafficHazard: Float = 0.0
        let playerPosition = currentPlayerPosition(state: state)
        vertices.reserveCapacity((dynamicProps.count + interestPoints.count * 2 + roadLinks.count * 3 + visibleBlockIndices.count * 8) * 36)

        for blockIndex in visibleBlockIndices.sorted() where blockIndex < blocks.count {
            let block = blocks[blockIndex]
            let profile = populationProfile(for: blockIndex, block: block, populationProfiles: populationProfiles)
            let incidentInfluence = incidentReactionIntensity(position: block.origin, state: state)
            let coolInfluence = streetCoolInfluence(position: block.origin, state: state)
            vertices.append(
                contentsOf: makeDistrictAmbientVertices(
                    block: block,
                    profile: profile,
                    elapsedTime: elapsedTime,
                    incidentInfluence: incidentInfluence,
                    normalizationInfluence: coolInfluence
                )
            )
        }

        for dynamicProp in dynamicProps where visibleBlockIndices.contains(Int(dynamicProp.blockIndex)) {
            vertices.append(contentsOf: makeDynamicPropVertices(dynamicProp, elapsedTime: elapsedTime))
        }

        for (linkIndex, link) in roadLinks.enumerated() {
            guard Int(link.fromBlockIndex) < blocks.count, Int(link.toBlockIndex) < blocks.count else {
                continue
            }

            guard visibleBlockIndices.contains(Int(link.fromBlockIndex)) || visibleBlockIndices.contains(Int(link.toBlockIndex)) else {
                continue
            }

            let incidentInfluence = roadLinkIncidentInfluence(link: link, blocks: blocks, state: state)
            let coolInfluence = streetCoolInfluence(position: link.midpoint, state: state)
            let isActiveLink = activeLinkIndex == UInt32(linkIndex)

            guard isActiveLink || incidentInfluence > 0.12 || coolInfluence > 0.10 else {
                continue
            }

            vertices.append(
                contentsOf: makeRoadLinkPulseVertices(
                    link: link,
                    blocks: blocks,
                    elapsedTime: elapsedTime + Float(linkIndex) * 0.18,
                    incidentInfluence: incidentInfluence,
                    normalizationInfluence: coolInfluence,
                    isActiveLink: isActiveLink
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
                    state: state,
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
                            elapsedTime: elapsedTime + Float(pointIndex) * 0.2 + sample.reactionIntensity * 0.24,
                            tint: sample.tint
                        )
                    )
                    if sample.reactionIntensity > 0.08 {
                        vertices.append(
                            contentsOf: makeWorldBoxVertices(
                                center: SIMD3<Float>(sample.position.x, sample.position.y + 0.04, sample.position.z),
                                halfExtents: SIMD3<Float>(
                                    0.28 + sample.reactionIntensity * 0.18,
                                    0.02,
                                    0.28 + sample.reactionIntensity * 0.18
                                ),
                                yaw: 0.0,
                                color: SIMD4<Float>(1.0, 0.58, 0.24, 0.16 + sample.reactionIntensity * 0.16)
                            )
                        )
                    }
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
                    trafficOccupancies: trafficOccupancies,
                    playerPosition: playerPosition,
                    traversalMode: state.traversal_mode,
                    elapsedTime: elapsedTime
                ) {
                    trafficHazard = max(trafficHazard, sample.hazardIntensity)
                    vertices.append(
                        contentsOf: makeVehiclePlaceholderVertices(
                            position: sample.position,
                            yaw: sample.yaw,
                            elapsedTime: elapsedTime + Float(pointIndex) * 0.35,
                            tint: sample.tint,
                            kind: sample.kind,
                            yieldIntensity: sample.yieldIntensity
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

        if trafficHazard > 0.12 {
            vertices.append(
                contentsOf: makeTrafficHazardVertices(
                    position: playerPosition,
                    intensity: trafficHazard,
                    traversalMode: state.traversal_mode,
                    elapsedTime: elapsedTime
                )
            )
        }

        return AmbientFrame(vertices: vertices, trafficHazard: trafficHazard)
    }

    private static func makeVehicleAnchorVertices(vehicleAnchors: [SceneVehicleAnchor], visibleBlockIndices: Set<Int>, state: MDTBEngineState, elapsedTime: Float, blocks: [SceneBlock]) -> [Vertex] {
        var vertices: [Vertex] = []
        let rankedIndices = rankedVehicleIndices(state: state, vehicleAnchors: vehicleAnchors)
        let playerPosition = currentPlayerPosition(state: state)

        for (anchorIndex, anchor) in vehicleAnchors.enumerated() {
            let blockIndex = Int(anchor.blockIndex)
            guard visibleBlockIndices.contains(blockIndex) else {
                continue
            }

            let block = blocks[safe: blockIndex]
            var tint = block.map(vehicleTint(for:)) ?? SIMD4<Float>(0.72, 0.42, 0.26, 1.0)
            let isActiveVehicle = state.traversal_mode == UInt32(MDTBTraversalModeVehicle) && state.active_vehicle_anchor_index == UInt32(anchorIndex)
            let isPreviewVehicle = state.traversal_mode == UInt32(MDTBTraversalModeOnFoot) && state.nearby_vehicle_anchor_index == UInt32(anchorIndex)
            let isLockedVehicle = state.traversal_mode == UInt32(MDTBTraversalModeOnFoot) &&
                state.vehicle_selection_locked != 0 &&
                state.locked_vehicle_anchor_index == UInt32(anchorIndex)
            let isMountReady = state.traversal_mode == UInt32(MDTBTraversalModeOnFoot) && sqrt(distanceSquared(playerPosition, anchor.position)) <= Self.vehicleMountRadius
            let rank = rankedIndices.firstIndex(of: anchorIndex)

            if isActiveVehicle {
                tint = animatedColor(tint, intensity: 1.18)
            } else if isLockedVehicle {
                tint = animatedColor(tint, intensity: 1.12)
            } else if isPreviewVehicle {
                tint = animatedColor(tint, intensity: 1.06)
            } else {
                tint = animatedColor(tint, intensity: 0.88)
            }

            let emphasis: Float = isActiveVehicle ? 1.0 : (isLockedVehicle ? 0.92 : (isPreviewVehicle ? 0.86 : (rank != nil ? 0.62 : 0.36)))
            vertices.append(
                contentsOf: makeVehiclePlaceholderVertices(
                    position: anchor.position,
                    yaw: anchor.yaw,
                    elapsedTime: elapsedTime + Float(anchorIndex) * 0.4,
                    tint: tint,
                    kind: anchor.kind,
                    yieldIntensity: 0.0
                )
            )

            vertices.append(
                contentsOf: makeVehicleParkingGuideVertices(
                    anchor: anchor,
                    elapsedTime: elapsedTime,
                    isActive: isActiveVehicle,
                    isNearby: isPreviewVehicle
                )
            )

            vertices.append(
                contentsOf: makeVehicleStagingMarkerVertices(
                    anchor: anchor,
                    elapsedTime: elapsedTime,
                    emphasis: emphasis,
                    rank: rank,
                    isSelected: isPreviewVehicle,
                    isLocked: isLockedVehicle,
                    isMountReady: isMountReady
                )
            )

            if isActiveVehicle || isPreviewVehicle || isLockedVehicle {
                let beaconBase = vehicleSignalColor(for: anchor.kind)
                let beaconColor = isActiveVehicle
                    ? animatedColor(beaconBase, intensity: 1.16)
                    : (isLockedVehicle
                        ? animatedColor(SIMD4<Float>(0.95, 0.91, 0.72, 1.0), intensity: isMountReady ? 1.02 : 0.92)
                        : animatedColor(beaconBase, intensity: isMountReady ? 1.02 : 0.86))
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

    private static func makeVehicleStagingMarkerVertices(anchor: SceneVehicleAnchor, elapsedTime: Float, emphasis: Float, rank: Int?, isSelected: Bool, isLocked: Bool, isMountReady: Bool) -> [Vertex] {
        let signalColor = animatedColor(vehicleSignalColor(for: anchor.kind), intensity: (isLocked ? 0.72 : 0.58) + emphasis * 0.64)
        let stemHeight: Float = anchor.kind == UInt32(MDTBVehicleKindBicycle) ? 0.52 : 0.72
        let topWidth: Float = anchor.kind == UInt32(MDTBVehicleKindBicycle) ? 0.16 : 0.22
        let bob = sin((elapsedTime * 2.1) + anchor.position.x * 0.02 + anchor.position.z * 0.03) * 0.04
        let rankBars = rank.map { max(1, 3 - $0) } ?? 0
        let readyLift: Float = isMountReady ? 0.12 : 0.0
        let stemColor = animatedColor(signalColor, intensity: isSelected ? 0.96 : (isLocked ? 1.02 : 0.82))
        var vertices =
            makeWorldBoxVertices(
                center: SIMD3<Float>(anchor.position.x, anchor.position.y + 0.34 + stemHeight * 0.5 + bob + readyLift, anchor.position.z),
                halfExtents: SIMD3<Float>(0.04, stemHeight * 0.5, 0.04),
                yaw: 0.0,
                color: stemColor
            ) +
            makeWorldBoxVertices(
                center: SIMD3<Float>(anchor.position.x, anchor.position.y + 0.40 + stemHeight + bob + readyLift, anchor.position.z),
                halfExtents: SIMD3<Float>(topWidth + (isSelected ? 0.04 : 0.0), 0.05, topWidth + (isSelected ? 0.04 : 0.0)),
                yaw: elapsedTime * 0.24,
                color: signalColor
            )

        if rankBars > 0 {
            for barIndex in 0 ..< rankBars {
                let offset = (Float(barIndex) - Float(rankBars - 1) * 0.5) * 0.24
                vertices.append(
                    contentsOf: makeWorldBoxVertices(
                        center: SIMD3<Float>(anchor.position.x + offset, anchor.position.y + 0.62 + stemHeight + bob + readyLift, anchor.position.z),
                        halfExtents: SIMD3<Float>(0.08, 0.03, 0.08),
                        yaw: elapsedTime * 0.18,
                        color: animatedColor(signalColor, intensity: isSelected ? 1.06 : 0.88)
                    )
                )
            }
        }

        if isLocked {
            let lockColor = animatedColor(SIMD4<Float>(0.95, 0.91, 0.72, 1.0), intensity: 0.92 + emphasis * 0.18)
            vertices.append(
                contentsOf: makeWorldBoxVertices(
                    center: SIMD3<Float>(anchor.position.x, anchor.position.y + 0.76 + stemHeight + bob + readyLift, anchor.position.z),
                    halfExtents: SIMD3<Float>(topWidth + 0.12, 0.03, 0.05),
                    yaw: elapsedTime * 0.14,
                    color: lockColor
                )
            )
            vertices.append(
                contentsOf: makeWorldBoxVertices(
                    center: SIMD3<Float>(anchor.position.x, anchor.position.y + 0.76 + stemHeight + bob + readyLift, anchor.position.z),
                    halfExtents: SIMD3<Float>(0.05, 0.03, topWidth + 0.12),
                    yaw: elapsedTime * 0.14,
                    color: lockColor
                )
            )
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

    private static func makeDistrictAmbientVertices(block: SceneBlock, profile: ScenePopulationProfile, elapsedTime: Float, incidentInfluence: Float, normalizationInfluence: Float) -> [Vertex] {
        let energy = max(profile.ambientEnergy, 0.25)
        let baseColor = districtAmbientColor(for: block)
        let spillColor = blendedColor(
            baseColor,
            SIMD4<Float>(1.0, 0.62, 0.24, 1.0),
            amount: min(max(incidentInfluence * 0.58, 0.0), 0.58)
        )
        let normalizeColor = blendedColor(
            spillColor,
            SIMD4<Float>(0.42, 0.86, 0.94, 1.0),
            amount: min(max(normalizationInfluence * 0.52, 0.0), 0.52)
        )
        let pulse = 0.74 + (sin((elapsedTime * (1.2 + energy)) + Float(block.variant) * 0.5) * 0.14 * energy)
        var vertices: [Vertex] = []

        if (block.tagMask & UInt32(MDTBBlockTagRetail)) != 0 {
            vertices.append(
                contentsOf: makeWorldBoxVertices(
                    center: SIMD3<Float>(block.origin.x + 6.0, block.origin.y + 2.26, block.origin.z - 13.0),
                    halfExtents: SIMD3<Float>(2.8, 0.12, 0.08),
                    yaw: 0.0,
                    color: animatedColor(normalizeColor, intensity: pulse + incidentInfluence * 0.10)
                )
            )
        }

        if (block.tagMask & UInt32(MDTBBlockTagTransit)) != 0 {
            let transitColor = animatedColor(normalizeColor, intensity: 0.82 + energy * 0.18 + incidentInfluence * 0.08)
            vertices.append(contentsOf: makeWorldBoxVertices(center: SIMD3<Float>(block.origin.x - 10.2, block.origin.y + 1.3, block.origin.z - 10.2), halfExtents: SIMD3<Float>(0.12, 0.90, 0.12), yaw: 0.0, color: transitColor))
            vertices.append(contentsOf: makeWorldBoxVertices(center: SIMD3<Float>(block.origin.x + 10.2, block.origin.y + 1.3, block.origin.z + 10.2), halfExtents: SIMD3<Float>(0.12, 0.90, 0.12), yaw: 0.0, color: transitColor))
        }

        if (block.tagMask & UInt32(MDTBBlockTagResidential)) != 0 {
            let porchBase = blendedColor(
                SIMD4<Float>(0.92, 0.80, 0.48, 1.0),
                SIMD4<Float>(1.0, 0.64, 0.30, 1.0),
                amount: min(max(incidentInfluence * 0.44, 0.0), 0.44)
            )
            let porchColor = animatedColor(porchBase, intensity: 0.70 + energy * 0.20 + normalizationInfluence * 0.06)
            vertices.append(contentsOf: makeWorldBoxVertices(center: SIMD3<Float>(block.origin.x + 38.2, block.origin.y + 2.0, block.origin.z + 33.8), halfExtents: SIMD3<Float>(0.86, 0.10, 0.10), yaw: 0.0, color: porchColor))
            vertices.append(contentsOf: makeWorldBoxVertices(center: SIMD3<Float>(block.origin.x - 41.6, block.origin.y + 3.0, block.origin.z + 15.0), halfExtents: SIMD3<Float>(1.2, 0.10, 0.10), yaw: 0.0, color: porchColor))
        }

        if (block.tagMask & UInt32(MDTBBlockTagCourt)) != 0 {
            let courtColor = animatedColor(normalizeColor, intensity: 0.66 + energy * 0.18 + incidentInfluence * 0.08)
            vertices.append(contentsOf: makeWorldBoxVertices(center: SIMD3<Float>(block.origin.x + 34.8, block.origin.y + 0.08, block.origin.z + 35.0), halfExtents: SIMD3<Float>(5.2, 0.03, 0.10), yaw: 0.0, color: courtColor))
        }

        if (profile.styleFlags & UInt32(MDTBPopulationStyleThroughTraffic)) != 0 {
            let laneColor = animatedColor(normalizeColor, intensity: 0.68 + energy * 0.14 + incidentInfluence * 0.10)
            vertices.append(contentsOf: makeWorldBoxVertices(center: SIMD3<Float>(block.origin.x, block.origin.y + 0.06, block.origin.z), halfExtents: SIMD3<Float>(0.10, 0.03, 6.4), yaw: .pi * 0.5, color: laneColor))
        }

        return vertices
    }

    private static func pedestrianSample(for point: SceneInterestPoint, pointIndex: Int, state: MDTBEngineState, block: SceneBlock, profile: ScenePopulationProfile, blocks: [SceneBlock], roadLinks: [SceneRoadLink], interestPoints: [SceneInterestPoint], elapsedTime: Float) -> PedestrianSample? {
        let districtTempo = 0.76 + (profile.ambientEnergy * 0.46)
        let activityBoost: Float = (profile.styleFlags & UInt32(MDTBPopulationStyleRetailClustered)) != 0 ? 0.06 : 0.0
        let incidentPosition = SIMD3<Float>(state.street_incident_position.x, state.street_incident_position.y, state.street_incident_position.z)
        let pointReaction = incidentReactionIntensity(position: point.position, state: state)
        let activeShare = min(0.88, 0.38 + (profile.pedestrianDensity * 0.42) + activityBoost) * (1.0 - pointReaction * 0.26)
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
        let basePosition = SIMD3<Float>(routeSample.position.x + (shoulder.x * sidewalkBias), point.position.y, routeSample.position.z + (shoulder.z * sidewalkBias))
        let reactionIntensity = min(max(pointReaction, 0.0), 1.0)
        let reactionTint = pedestrianTint(for: block)
        let awayVector = SIMD3<Float>(basePosition.x - incidentPosition.x, 0.0, basePosition.z - incidentPosition.z)
        let awayLengthSquared = max((awayVector.x * awayVector.x) + (awayVector.z * awayVector.z), 0.0001)
        let awayScale = reactionIntensity > 0.0 ? (1.0 / sqrt(awayLengthSquared)) : 0.0
        let awayDirection = SIMD3<Float>(awayVector.x * awayScale, 0.0, awayVector.z * awayScale)
        let reactionPosition = reactionIntensity > 0.08
            ? SIMD3<Float>(
                basePosition.x + awayDirection.x * (0.72 + reactionIntensity * 1.60),
                point.position.y,
                basePosition.z + awayDirection.z * (0.72 + reactionIntensity * 1.60)
            )
            : basePosition
        let heading = reactionIntensity > 0.08
            ? atan2(awayDirection.x, -awayDirection.z)
            : routeSample.heading
        let tint = reactionIntensity > 0.0
            ? SIMD4<Float>(
                min(reactionTint.x * (0.86 + reactionIntensity * 0.44), 1.0),
                min((reactionTint.y * (0.74 + reactionIntensity * 0.18)) + reactionIntensity * 0.24, 1.0),
                min((reactionTint.z * (0.66 - reactionIntensity * 0.10)) + reactionIntensity * 0.12, 1.0),
                1.0
            )
            : reactionTint

        return PedestrianSample(
            position: reactionPosition,
            heading: heading,
            tint: tint,
            reactionIntensity: reactionIntensity
        )
    }

    private static func vehicleSample(for point: SceneInterestPoint, pointIndex: Int, block: SceneBlock, profile: ScenePopulationProfile, blocks: [SceneBlock], roadLinks: [SceneRoadLink], trafficOccupancies: [SceneTrafficOccupancy], playerPosition: SIMD3<Float>, traversalMode: UInt32, elapsedTime: Float) -> VehicleSample? {
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
        let vehicleKind = trafficVehicleKind(for: pointIndex, block: block, profile: profile)
        let laneMagnitude = trafficLaneMagnitude(for: vehicleKind)
        guard let adjustedSample = trafficAdjustedSample(
            routePoints: routePoints,
            baseProgress: progress,
            laneMagnitude: laneMagnitude,
            vehicleKind: vehicleKind,
            trafficOccupancies: trafficOccupancies
        ) else {
            return nil
        }

        let samplePosition = SIMD3<Float>(adjustedSample.position.x, point.position.y, adjustedSample.position.z)

        return VehicleSample(
            position: samplePosition,
            yaw: adjustedSample.heading,
            tint: vehicleTint(for: block),
            kind: vehicleKind,
            yieldIntensity: adjustedSample.yieldIntensity,
            hazardIntensity: trafficHazardIntensity(
                samplePosition: samplePosition,
                playerPosition: playerPosition,
                traversalMode: traversalMode
            )
        )
    }

    private static func trafficAdjustedSample(routePoints: [SIMD3<Float>], baseProgress: Float, laneMagnitude: Float, vehicleKind: UInt32, trafficOccupancies: [SceneTrafficOccupancy]) -> (position: SIMD3<Float>, heading: Float, yieldIntensity: Float)? {
        var progress = baseProgress
        let clearance = trafficOccupancyRadius(for: vehicleKind)
        var yieldIntensity: Float = 0.0

        for _ in 0 ..< 4 {
            let routeSample = sampleRoute(points: routePoints, progress: progress)
            let travelAxis = roadAxis(for: routeSample.heading)
            let laneRight = SIMD3<Float>(cos(routeSample.heading), 0.0, sin(routeSample.heading))
            let laneOffset = laneRight * trafficLaneOffset(forHeading: routeSample.heading, laneMagnitude: laneMagnitude)
            let position = SIMD3<Float>(routeSample.position.x + laneOffset.x, routeSample.position.y, routeSample.position.z + laneOffset.z)
            let occupancyPressure = trafficOccupancies.compactMap { occupancy -> (distance: Float, pressure: Float, isStopZone: Bool, isIncident: Bool)? in
                let blockerDistance = sqrt(distanceSquared(position, occupancy.position))
                let axisAligned = occupancy.axis == travelAxis
                let isStopZone = occupancy.reason == UInt32(MDTBTrafficOccupancyReasonStopZone) && axisAligned
                let isIncident = occupancy.reason == UInt32(MDTBTrafficOccupancyReasonIncident)
                let softRadius =
                    clearance +
                    occupancy.radius +
                    (isStopZone ? 2.1 : (isIncident ? 1.7 : 1.2)) +
                    occupancy.strength * (isStopZone ? 2.4 : (isIncident ? (axisAligned ? 2.2 : 1.5) : (axisAligned ? 1.8 : 1.1)))
                guard blockerDistance < softRadius else {
                    return nil
                }

                var pressure = (1.0 - max(0.0, min((blockerDistance - clearance) / max(softRadius - clearance, 0.01), 1.0))) * occupancy.strength
                if isStopZone {
                    pressure *= 1.22
                } else if isIncident && axisAligned {
                    pressure *= 1.10
                } else if !axisAligned {
                    pressure *= isIncident ? 0.92 : 0.86
                }

                return (distance: blockerDistance, pressure: pressure, isStopZone: isStopZone, isIncident: isIncident)
            }.max(by: { lhs, rhs in
                lhs.pressure < rhs.pressure
            })

            guard let occupancyPressure else {
                return (position, routeSample.heading, yieldIntensity)
            }

            let blockerDistance = occupancyPressure.distance
            let pressure = occupancyPressure.pressure
            yieldIntensity = max(
                yieldIntensity,
                occupancyPressure.isStopZone
                    ? min(1.0, pressure + 0.18)
                    : (occupancyPressure.isIncident ? min(1.0, pressure + 0.10) : pressure)
            )

            if occupancyPressure.isStopZone {
                if blockerDistance <= clearance + 0.45 && progress <= 0.08 {
                    return nil
                }

                progress = max(progress - (0.14 + pressure * 0.30), 0.0)
            } else {
                if blockerDistance <= clearance + (occupancyPressure.isIncident ? 0.25 : 0.0) && progress <= 0.02 {
                    return nil
                }

                progress = max(
                    progress - ((occupancyPressure.isIncident ? 0.10 : 0.08) + pressure * (occupancyPressure.isIncident ? 0.28 : 0.18)),
                    0.0
                )
            }
        }

        let routeSample = sampleRoute(points: routePoints, progress: progress)
        let laneRight = SIMD3<Float>(cos(routeSample.heading), 0.0, sin(routeSample.heading))
        let laneOffset = laneRight * trafficLaneOffset(forHeading: routeSample.heading, laneMagnitude: laneMagnitude)
        let position = SIMD3<Float>(routeSample.position.x + laneOffset.x, routeSample.position.y, routeSample.position.z + laneOffset.z)
        return (position, routeSample.heading, yieldIntensity)
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
        case UInt32(MDTBDistrictWestAdams), UInt32(MDTBDistrictJeffersonPark), UInt32(MDTBDistrictPicoUnion):
            return SIMD4<Float>(0.84, 0.61, 0.28, 1.0)
        case UInt32(MDTBDistrictExpositionPark), UInt32(MDTBDistrictLeimertPark), UInt32(MDTBDistrictInglewood):
            return SIMD4<Float>(0.50, 0.67, 0.38, 1.0)
        case UInt32(MDTBDistrictCrenshawCorridor), UInt32(MDTBDistrictUniversityPark):
            return SIMD4<Float>(0.28, 0.66, 0.74, 1.0)
        case UInt32(MDTBDistrictVermontSquare), UInt32(MDTBDistrictKoreatown):
            return SIMD4<Float>(0.43, 0.57, 0.78, 1.0)
        case UInt32(MDTBDistrictHistoricSouthCentral), UInt32(MDTBDistrictSouthPark):
            return SIMD4<Float>(0.78, 0.48, 0.30, 1.0)
        case UInt32(MDTBDistrictFlorenceFirestone), UInt32(MDTBDistrictHuntingtonPark), UInt32(MDTBDistrictWatts), UInt32(MDTBDistrictWillowbrook):
            return SIMD4<Float>(0.70, 0.46, 0.36, 1.0)
        default:
            return SIMD4<Float>(0.79, 0.40, 0.28, 1.0)
        }
    }

    private static func vehicleTint(for block: SceneBlock) -> SIMD4<Float> {
        switch block.district {
        case UInt32(MDTBDistrictWestAdams), UInt32(MDTBDistrictJeffersonPark), UInt32(MDTBDistrictPicoUnion):
            return SIMD4<Float>(0.76, 0.48, 0.22, 1.0)
        case UInt32(MDTBDistrictExpositionPark), UInt32(MDTBDistrictLeimertPark), UInt32(MDTBDistrictInglewood):
            return SIMD4<Float>(0.32, 0.53, 0.30, 1.0)
        case UInt32(MDTBDistrictCrenshawCorridor), UInt32(MDTBDistrictUniversityPark):
            return SIMD4<Float>(0.26, 0.60, 0.70, 1.0)
        case UInt32(MDTBDistrictVermontSquare), UInt32(MDTBDistrictKoreatown):
            return SIMD4<Float>(0.34, 0.50, 0.76, 1.0)
        case UInt32(MDTBDistrictHistoricSouthCentral), UInt32(MDTBDistrictSouthPark):
            return SIMD4<Float>(0.64, 0.40, 0.24, 1.0)
        case UInt32(MDTBDistrictFlorenceFirestone), UInt32(MDTBDistrictHuntingtonPark), UInt32(MDTBDistrictWatts), UInt32(MDTBDistrictWillowbrook):
            return SIMD4<Float>(0.56, 0.42, 0.38, 1.0)
        default:
            return SIMD4<Float>(0.68, 0.40, 0.24, 1.0)
        }
    }

    private static func trafficVehicleKind(for pointIndex: Int, block: SceneBlock, profile: ScenePopulationProfile) -> UInt32 {
        if block.frontageTemplate == UInt32(MDTBFrontageTemplateTransitMarket) {
            return pointIndex % 4 == 0 ? UInt32(MDTBVehicleKindMotorcycle) : (pointIndex % 2 == 0 ? UInt32(MDTBVehicleKindMoped) : UInt32(MDTBVehicleKindCoupe))
        }

        if block.frontageTemplate == UInt32(MDTBFrontageTemplateResidentialCourt) {
            return pointIndex % 3 == 0 ? UInt32(MDTBVehicleKindBicycle) : (pointIndex % 2 == 0 ? UInt32(MDTBVehicleKindCoupe) : UInt32(MDTBVehicleKindSedan))
        }

        if (profile.styleFlags & UInt32(MDTBPopulationStyleThroughTraffic)) != 0 {
            return pointIndex % 5 == 0 ? UInt32(MDTBVehicleKindMotorcycle) : (pointIndex % 2 == 0 ? UInt32(MDTBVehicleKindCoupe) : UInt32(MDTBVehicleKindSedan))
        }

        return UInt32(MDTBVehicleKindSedan)
    }

    private static func trafficLaneMagnitude(for kind: UInt32) -> Float {
        switch kind {
        case UInt32(MDTBVehicleKindBicycle):
            return 1.12
        case UInt32(MDTBVehicleKindMoped):
            return 1.28
        case UInt32(MDTBVehicleKindMotorcycle):
            return 1.46
        case UInt32(MDTBVehicleKindCoupe):
            return 1.60
        default:
            return 1.72
        }
    }

    private static func trafficOccupancyRadius(for kind: UInt32) -> Float {
        switch kind {
        case UInt32(MDTBVehicleKindBicycle):
            return 2.1
        case UInt32(MDTBVehicleKindMoped):
            return 2.4
        case UInt32(MDTBVehicleKindMotorcycle):
            return 2.8
        case UInt32(MDTBVehicleKindCoupe):
            return 3.1
        default:
            return 3.4
        }
    }

    private static func roadAxis(for heading: Float) -> UInt32 {
        abs(cos(heading)) >= abs(sin(heading)) ? UInt32(MDTBRoadAxisNorthSouth) : UInt32(MDTBRoadAxisEastWest)
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

    private static func blendedColor(_ lhs: SIMD4<Float>, _ rhs: SIMD4<Float>, amount: Float) -> SIMD4<Float> {
        let t = min(max(amount, 0.0), 1.0)
        return lhs + ((rhs - lhs) * t)
    }

    private static func makeVehicleLocalPartVertices(position: SIMD3<Float>, localCenter: SIMD3<Float>, halfExtents: SIMD3<Float>, yaw: Float, color: SIMD4<Float>) -> [Vertex] {
        makeActorPartVertices(
            position: position,
            localCenter: localCenter,
            halfExtents: halfExtents,
            bodyYaw: yaw,
            partYaw: yaw,
            color: color
        )
    }

    private static func makeVehicleWheelVertices(position: SIMD3<Float>, yaw: Float, localCenter: SIMD3<Float>, axleHalfWidth: Float, radius: Float, treadHalfDepth: Float, tireColor: SIMD4<Float>, rimColor: SIMD4<Float>) -> [Vertex] {
        let capYOffset = radius * 0.54
        let sideZOffset = treadHalfDepth * 0.44

        return
            makeVehicleLocalPartVertices(
                position: position,
                localCenter: localCenter,
                halfExtents: SIMD3<Float>(axleHalfWidth, radius * 0.42, treadHalfDepth * 0.76),
                yaw: yaw,
                color: tireColor
            ) +
            makeVehicleLocalPartVertices(
                position: position,
                localCenter: localCenter + SIMD3<Float>(0.0, capYOffset, 0.0),
                halfExtents: SIMD3<Float>(axleHalfWidth * 0.92, radius * 0.16, treadHalfDepth * 0.54),
                yaw: yaw,
                color: tireColor
            ) +
            makeVehicleLocalPartVertices(
                position: position,
                localCenter: localCenter - SIMD3<Float>(0.0, capYOffset, 0.0),
                halfExtents: SIMD3<Float>(axleHalfWidth * 0.92, radius * 0.16, treadHalfDepth * 0.54),
                yaw: yaw,
                color: tireColor
            ) +
            makeVehicleLocalPartVertices(
                position: position,
                localCenter: localCenter + SIMD3<Float>(0.0, 0.0, sideZOffset),
                halfExtents: SIMD3<Float>(axleHalfWidth * 0.88, radius * 0.28, treadHalfDepth * 0.22),
                yaw: yaw,
                color: tireColor
            ) +
            makeVehicleLocalPartVertices(
                position: position,
                localCenter: localCenter - SIMD3<Float>(0.0, 0.0, sideZOffset),
                halfExtents: SIMD3<Float>(axleHalfWidth * 0.88, radius * 0.28, treadHalfDepth * 0.22),
                yaw: yaw,
                color: tireColor
            ) +
            makeVehicleLocalPartVertices(
                position: position,
                localCenter: localCenter,
                halfExtents: SIMD3<Float>(axleHalfWidth * 0.56, radius * 0.18, treadHalfDepth * 0.34),
                yaw: yaw,
                color: rimColor
            )
    }

    private static func makeVehicleLightPairVertices(position: SIMD3<Float>, yaw: Float, lateralOffset: Float, localY: Float, localZ: Float, halfExtents: SIMD3<Float>, color: SIMD4<Float>) -> [Vertex] {
        makeVehicleLocalPartVertices(
            position: position,
            localCenter: SIMD3<Float>(-lateralOffset, localY, localZ),
            halfExtents: halfExtents,
            yaw: yaw,
            color: color
        ) +
        makeVehicleLocalPartVertices(
            position: position,
            localCenter: SIMD3<Float>(lateralOffset, localY, localZ),
            halfExtents: halfExtents,
            yaw: yaw,
            color: color
        )
    }

    private static func makePedestrianPlaceholderVertices(position: SIMD3<Float>, heading: Float, elapsedTime: Float, tint: SIMD4<Float>) -> [Vertex] {
        let variant = pedestrianMeshVariant(position: position)
        return artMeshes.makeVertices(
            for: variant,
            position: position,
            yaw: heading,
            scale: SIMD3<Float>(repeating: 0.82),
            tint: tint,
            lift: sin(elapsedTime * 3.2) * 0.04
        )
    }

    private static func makeVehiclePlaceholderVertices(position: SIMD3<Float>, yaw: Float, elapsedTime: Float, tint: SIMD4<Float>, kind: UInt32, yieldIntensity: Float) -> [Vertex] {
        let (meshID, scale) = vehicleMeshDescriptor(for: kind)
        let liveTint = animatedColor(tint, intensity: 0.90 + yieldIntensity * 0.14 + sin(elapsedTime * 3.6) * 0.04)
        return artMeshes.makeVertices(
            for: meshID,
            position: position,
            yaw: yaw,
            scale: scale,
            tint: liveTint
        )
    }

    private static func makeTrafficHazardVertices(position: SIMD3<Float>, intensity: Float, traversalMode: UInt32, elapsedTime: Float) -> [Vertex] {
        let clamped = max(0.0, min(intensity, 1.0))
        let pulse = 0.68 + sin(elapsedTime * 7.2) * 0.16
        let radius: Float = traversalMode == UInt32(MDTBTraversalModeVehicle) ? 1.48 : 0.82
        let height: Float = traversalMode == UInt32(MDTBTraversalModeVehicle) ? 0.36 : 0.24
        let color = SIMD4<Float>(0.98, 0.26 + pulse * 0.12, 0.18, 1.0)
        var vertices: [Vertex] = []

        for angle in stride(from: Float(0.0), to: Float.pi * 2.0, by: Float.pi / 2.0) {
            let center = SIMD3<Float>(
                position.x + cos(angle) * (radius + clamped * 0.26),
                position.y + height + clamped * 0.18,
                position.z + sin(angle) * (radius + clamped * 0.26)
            )
            vertices.append(
                contentsOf: makeWorldBoxVertices(
                    center: center,
                    halfExtents: SIMD3<Float>(0.10 + clamped * 0.04, 0.04, 0.22 + clamped * 0.08),
                    yaw: angle + elapsedTime * 0.18,
                    color: animatedColor(color, intensity: 0.72 + clamped * 0.42)
                )
            )
        }

        return vertices
    }

    private static func makeRoadLinkPulseVertices(link: SceneRoadLink, blocks: [SceneBlock], elapsedTime: Float, incidentInfluence: Float, normalizationInfluence: Float, isActiveLink: Bool) -> [Vertex] {
        guard Int(link.fromBlockIndex) < blocks.count, Int(link.toBlockIndex) < blocks.count else {
            return []
        }

        let start = blocks[Int(link.fromBlockIndex)].origin
        let end = blocks[Int(link.toBlockIndex)].origin
        let yaw: Float = link.axis == UInt32(MDTBRoadAxisNorthSouth) ? 0.0 : (.pi * 0.5)
        let pulse = 0.70 + (sin(elapsedTime * 2.6) * 0.18)
        let baseColor: SIMD4<Float>
        var vertices: [Vertex] = []

        if incidentInfluence >= normalizationInfluence && incidentInfluence > 0.08 {
            baseColor = animatedColor(
                SIMD4<Float>(1.0, 0.62, 0.24, 1.0),
                intensity: pulse + incidentInfluence * (isActiveLink ? 0.30 : 0.20)
            )
        } else if normalizationInfluence > 0.08 {
            baseColor = animatedColor(
                SIMD4<Float>(0.36, 0.88, 0.94, 1.0),
                intensity: 0.72 + normalizationInfluence * (isActiveLink ? 0.22 : 0.14)
            )
        } else {
            baseColor = SIMD4<Float>(0.24, pulse, 0.90, 1.0)
        }

        for step in 1 ... 3 {
            let t = Float(step) / 4.0
            let position = start + ((end - start) * t)
            vertices.append(
                contentsOf: makeWorldBoxVertices(
                    center: SIMD3<Float>(position.x, 0.18, position.z),
                    halfExtents: SIMD3<Float>(0.18 + incidentInfluence * 0.04, 0.05, 1.35 + incidentInfluence * 0.12 + normalizationInfluence * 0.06),
                    yaw: yaw,
                    color: baseColor
                )
            )
        }

        return vertices
    }

    private static func makeCombatVertices(state: MDTBEngineState, elapsedTime: Float) -> [Vertex] {
        let pipePickupPosition = SIMD3<Float>(
            state.melee_weapon_pickup_position.x,
            state.melee_weapon_pickup_position.y,
            state.melee_weapon_pickup_position.z
        )
        let pistolPickupPosition = SIMD3<Float>(
            state.firearm_pickup_position.x,
            state.firearm_pickup_position.y,
            state.firearm_pickup_position.z
        )
        let dummyPosition = SIMD3<Float>(
            state.combat_target_position.x,
            state.combat_target_position.y,
            state.combat_target_position.z
        )
        let hostilePosition = SIMD3<Float>(
            state.combat_hostile_position.x,
            state.combat_hostile_position.y,
            state.combat_hostile_position.z
        )
        let witnessPosition = SIMD3<Float>(
            state.witness_position.x,
            state.witness_position.y,
            state.witness_position.z
        )
        let bystanderPosition = SIMD3<Float>(
            state.bystander_position.x,
            state.bystander_position.y,
            state.bystander_position.z
        )
        let searchPosition = SIMD3<Float>(
            state.combat_hostile_search_position.x,
            state.combat_hostile_search_position.y,
            state.combat_hostile_search_position.z
        )
        let incidentPosition = SIMD3<Float>(
            state.street_incident_position.x,
            state.street_incident_position.y,
            state.street_incident_position.z
        )
        let playerPosition = currentPlayerPosition(state: state)
        let playerFocus = SIMD3<Float>(
            playerPosition.x,
            state.actor_ground_height + 1.02,
            playerPosition.z
        )
        let shotFrom = SIMD3<Float>(
            state.firearm_last_shot_from.x,
            state.firearm_last_shot_from.y,
            state.firearm_last_shot_from.z
        )
        let shotTo = SIMD3<Float>(
            state.firearm_last_shot_to.x,
            state.firearm_last_shot_to.y,
            state.firearm_last_shot_to.z
        )
        let hostileShotFrom = SIMD3<Float>(
            state.combat_hostile_last_shot_from.x,
            state.combat_hostile_last_shot_from.y,
            state.combat_hostile_last_shot_from.z
        )
        let hostileShotTo = SIMD3<Float>(
            state.combat_hostile_last_shot_to.x,
            state.combat_hostile_last_shot_to.y,
            state.combat_hostile_last_shot_to.z
        )
        let dummyReaction = min(max(state.combat_target_reaction, 0.0), 1.4)
        let hostileReaction = min(max(state.combat_hostile_reaction, 0.0), 1.5)
        let dummyHealthRatio = max(0.0, min(state.combat_target_health / 100.0, 1.0))
        let hostileHealthRatio = max(0.0, min(state.combat_hostile_health / 84.0, 1.0))
        let playerHealthRatio = max(0.0, min(state.player_health / 100.0, 1.0))
        let dummyInRange = state.combat_target_in_range != 0
        let hostileInRange = state.combat_hostile_in_range != 0
        let dummyDowned = state.combat_target_health <= 0.0 && state.combat_target_reset_timer > 0.0
        let hostileDowned = state.combat_hostile_health <= 0.0 && state.combat_hostile_reset_timer > 0.0
        let hostileAlert = min(max(state.combat_hostile_alert, 0.0), 1.0)
        let witnessAlert = min(max(state.witness_alert, 0.0), 1.0)
        let bystanderAlert = min(max(state.bystander_alert, 0.0), 1.0)
        let streetIncidentLevel = min(max(state.street_incident_level, 0.0), 1.0)
        let territoryClaimed =
            state.territory_faction == UInt32(MDTBTerritoryFactionCourtSet) &&
            state.territory_phase != UInt32(MDTBTerritoryPhaseNone)
        let territoryHot = state.territory_phase == UInt32(MDTBTerritoryPhaseHot)
        let territoryDeepWatch = min(max(state.territory_deep_watch, 0.0), 1.0)
        let territoryWatchTimer = min(max(state.territory_watch_timer / 4.2, 0.0), 1.0)
        let territoryPatrolPosition = SIMD3<Float>(
            state.territory_patrol_position.x,
            state.territory_patrol_position.y,
            state.territory_patrol_position.z
        )
        let territoryPatrolHeading = state.territory_patrol_heading
        let territoryPatrolAlert = min(max(state.territory_patrol_alert, 0.0), 1.0)
        let territoryPatrolState = state.territory_patrol_state
        let territoryPatrolBrace = state.territory_patrol_state == UInt32(MDTBTerritoryPatrolBrace) && territoryPatrolAlert > 0.12
        let territoryPatrolClear = state.territory_patrol_state == UInt32(MDTBTerritoryPatrolClear) && territoryPatrolAlert > 0.12
        let territoryInnerPosition = SIMD3<Float>(
            state.territory_inner_position.x,
            state.territory_inner_position.y,
            state.territory_inner_position.z
        )
        let territoryInnerHeading = state.territory_inner_heading
        let territoryInnerAlert = min(max(state.territory_inner_alert, 0.0), 1.0)
        let territoryInnerState = state.territory_inner_state
        let territoryInnerVisual = max(
            territoryDeepWatch,
            territoryInnerAlert + (territoryInnerState == UInt32(MDTBTerritoryPatrolHandoff) ? 0.08 : 0.0)
        )
        let territoryEntryClamp = Self.territoryEntryClamped(state: state)
        let territoryCommitState = state.territory_commit_state
        let territoryCommitTimer = max(state.territory_commit_timer, 0.0)
        let territoryCommitProgress = min(max(state.territory_commit_progress, 0.0), 1.0)
        let territoryCommitWindow = territoryCommitState == UInt32(MDTBTerritoryCommitWindow) && territoryCommitTimer > 0.05
        let territoryCommitActive = territoryCommitState == UInt32(MDTBTerritoryCommitActive) && territoryCommitProgress > 0.01
        let territoryCommitComplete = territoryCommitState == UInt32(MDTBTerritoryCommitComplete) && territoryCommitTimer > 0.05
        let territoryResolveState = state.territory_resolve_state
        let territoryResolveTimer = max(state.territory_resolve_timer, 0.0)
        let territoryResolveProgress = min(max(state.territory_resolve_progress, 0.0), 1.0)
        let territoryResolveWindow = territoryResolveState == UInt32(MDTBTerritoryResolveWindow) && territoryResolveTimer > 0.05
        let territoryResolveHold = territoryResolveState == UInt32(MDTBTerritoryResolveHold) && territoryResolveTimer > 0.05
        let territoryResolvePullout = territoryResolveState == UInt32(MDTBTerritoryResolvePullout) && territoryResolveTimer > 0.05
        let territoryResolveHeld = territoryResolveHold && territoryResolveProgress > 0.99
        let territoryResolveClean = territoryResolvePullout && territoryResolveProgress > 0.99
        let territoryReclaimReturn = Self.territoryReclaimReturnActive(state: state)
        let territoryRetakeReturn = Self.territoryRetakeReturnActive(state: state)
        let territoryReclaimFeint = Self.territoryReclaimFeint(state: state)
        let territoryReclaimCommit = Self.territoryReclaimCommit(state: state)
        let territoryReclaimCounterStep = Self.territoryReclaimCounterStep(state: state)
        let territoryRetakeChallenge = Self.territoryRetakeChallenge(state: state)
        let territoryRetakeCommit = Self.territoryRetakeCommit(state: state)
        let territoryRetakeCrossAngle = Self.territoryRetakeCrossAngle(state: state)
        let territoryReturnPulse = 0.78 + (sin(elapsedTime * 5.2) * 0.16)
        let territoryReturnStrength = min(max(state.territory_reapproach_timer / 5.4, 0.0), 1.0)
        let playerInCover = state.combat_player_in_cover != 0
        let focusOccluded = state.combat_focus_occluded != 0
        let playerDamagePulse = min(max(state.player_damage_pulse, 0.0), 1.0)
        let playerResetPulse = min(max(state.player_reset_timer / 1.55, 0.0), 1.0)
        let hostileAttackCharge = state.combat_hostile_attack_windup > 0.0
            ? 1.0 - min(max(state.combat_hostile_attack_windup / 0.42, 0.0), 1.0)
            : 0.0
        let focusKind = state.combat_focus_target_kind
        let hitKind = state.combat_last_hit_target_kind
        let witnessState = state.witness_state
        let bystanderState = state.bystander_state
        let pickupPulse = 0.78 + (sin(elapsedTime * 5.4) * 0.18)
        let shotPulse = min(max(state.firearm_last_shot_timer / 0.10, 0.0), 1.0)
        let hostileShotPulse = min(max(state.combat_hostile_last_shot_timer / 0.14, 0.0), 1.0)
        let searchCharge = min(max(state.combat_hostile_search_timer / 2.2, 0.0), 1.0)
        let searchPulse = 0.78 + (sin(elapsedTime * 6.6) * 0.18)
        let searchActive = state.combat_hostile_search_timer > 0.0
        let streetIncidentActive = state.street_incident_timer > 0.0 && streetIncidentLevel > 0.04
        let incidentPulse = 0.76 + (sin(elapsedTime * 4.8) * 0.16)
        let pipePickupColor = SIMD4<Float>(0.77, 0.72, 0.66, 1.0)
        let pipePickupHalo = SIMD4<Float>(0.94, 0.80, 0.30, 0.42)
        let pistolPickupHalo = SIMD4<Float>(0.38, 0.70, 1.0, 0.38)
        let dummyBodyColor = SIMD4<Float>(
            0.30 + ((1.0 - dummyHealthRatio) * 0.36) + (dummyReaction * 0.16),
            0.38 + (dummyHealthRatio * 0.24),
            0.46 - ((1.0 - dummyHealthRatio) * 0.16),
            1.0
        )
        let dummyAccentColor = SIMD4<Float>(0.92, 0.52 + dummyHealthRatio * 0.18, 0.22 + dummyReaction * 0.12, 1.0)
        let hostileBodyColor = SIMD4<Float>(
            0.34 + hostileAlert * 0.28,
            0.22 + hostileHealthRatio * 0.30,
            0.20 + hostileReaction * 0.12,
            1.0
        )
        let hostileAccentColor = SIMD4<Float>(1.0, 0.40 + hostileAlert * 0.36, 0.18 + hostileReaction * 0.18, 1.0)
        let streetIncidentColor = SIMD4<Float>(
            0.96,
            0.42 + streetIncidentLevel * 0.20,
            0.18 + streetIncidentLevel * 0.12,
            0.24 + streetIncidentLevel * 0.20
        )
        let witnessBodyColor: SIMD4<Float>
        let witnessAccentColor: SIMD4<Float>
        let witnessRingColor: SIMD4<Float>
        let bystanderBodyColor: SIMD4<Float>
        let bystanderAccentColor: SIMD4<Float>
        let bystanderRingColor: SIMD4<Float>

        switch witnessState {
        case UInt32(MDTBWitnessStateInvestigate):
            witnessBodyColor = SIMD4<Float>(0.64, 0.52 + witnessAlert * 0.10, 0.28, 1.0)
            witnessAccentColor = SIMD4<Float>(1.0, 0.78, 0.28, 1.0)
            witnessRingColor = SIMD4<Float>(0.96, 0.74, 0.28, 0.28 + witnessAlert * 0.18)
        case UInt32(MDTBWitnessStateFlee):
            witnessBodyColor = SIMD4<Float>(0.76, 0.28 + witnessAlert * 0.08, 0.24, 1.0)
            witnessAccentColor = SIMD4<Float>(1.0, 0.54, 0.24, 1.0)
            witnessRingColor = SIMD4<Float>(1.0, 0.40, 0.24, 0.32 + witnessAlert * 0.22)
        case UInt32(MDTBWitnessStateCooldown):
            witnessBodyColor = SIMD4<Float>(0.28, 0.50 + witnessAlert * 0.08, 0.46, 1.0)
            witnessAccentColor = SIMD4<Float>(0.60, 0.88, 0.80, 1.0)
            witnessRingColor = SIMD4<Float>(0.34, 0.74, 0.68, 0.24 + witnessAlert * 0.14)
        default:
            witnessBodyColor = SIMD4<Float>(0.38, 0.52, 0.62, 1.0)
            witnessAccentColor = SIMD4<Float>(0.74, 0.86, 0.94, 1.0)
            witnessRingColor = SIMD4<Float>(0.36, 0.62, 0.84, 0.20 + witnessAlert * 0.12)
        }

        switch bystanderState {
        case UInt32(MDTBWitnessStateInvestigate):
            bystanderBodyColor = SIMD4<Float>(0.56, 0.46 + bystanderAlert * 0.10, 0.64, 1.0)
            bystanderAccentColor = SIMD4<Float>(0.96, 0.74, 1.0, 1.0)
            bystanderRingColor = SIMD4<Float>(0.84, 0.58, 1.0, 0.24 + bystanderAlert * 0.16)
        case UInt32(MDTBWitnessStateFlee):
            bystanderBodyColor = SIMD4<Float>(0.84, 0.34 + bystanderAlert * 0.08, 0.46, 1.0)
            bystanderAccentColor = SIMD4<Float>(1.0, 0.58, 0.78, 1.0)
            bystanderRingColor = SIMD4<Float>(1.0, 0.44, 0.62, 0.30 + bystanderAlert * 0.20)
        case UInt32(MDTBWitnessStateCooldown):
            bystanderBodyColor = SIMD4<Float>(0.34, 0.48, 0.60 + bystanderAlert * 0.08, 1.0)
            bystanderAccentColor = SIMD4<Float>(0.72, 0.82, 1.0, 1.0)
            bystanderRingColor = SIMD4<Float>(0.44, 0.66, 0.96, 0.22 + bystanderAlert * 0.12)
        default:
            bystanderBodyColor = SIMD4<Float>(0.46, 0.50, 0.62, 1.0)
            bystanderAccentColor = SIMD4<Float>(0.84, 0.88, 1.0, 1.0)
            bystanderRingColor = SIMD4<Float>(0.50, 0.64, 0.92, 0.18 + bystanderAlert * 0.10)
        }
        let playerRingColor = playerResetPulse > 0.0
            ? SIMD4<Float>(0.52, 0.90, 0.54, 0.60 + playerResetPulse * 0.18)
            : (playerInCover
                ? SIMD4<Float>(0.30, 0.78, 0.94, 0.46)
                : SIMD4<Float>(0.98, 0.48 + playerHealthRatio * 0.22, 0.24, 0.44 + (1.0 - playerHealthRatio) * 0.20))
        let hostileAnchorPositions = [
            SIMD3<Float>(-13.8, 0.22, 52.0),
            SIMD3<Float>(-1.9, 0.22, 55.0),
            SIMD3<Float>(9.4, 0.22, 49.7),
            SIMD3<Float>(5.4, 0.22, 56.6),
        ]
        let hostileAnchorIndex = min(max(Int(state.combat_hostile_anchor_index), 0), hostileAnchorPositions.count - 1)
        let hostileAnchor = hostileAnchorPositions[hostileAnchorIndex]
        let hostileReacquirePulse = min(max(state.combat_hostile_reacquire_timer / 1.10, 0.0), 1.0)
        let hostileAnchorTravel = min(max(sqrt(distanceSquared(hostileAnchor, hostilePosition)) / 6.0, 0.0), 1.0)
        let hostileHeading = state.combat_hostile_heading
        let witnessHeading = state.witness_heading
        let bystanderHeading = state.bystander_heading
        let witnessRight = SIMD3<Float>(cos(witnessHeading), 0.0, sin(witnessHeading))
        let bystanderRight = SIMD3<Float>(cos(bystanderHeading), 0.0, sin(bystanderHeading))
        let territoryPatrolRight = SIMD3<Float>(cos(territoryPatrolHeading), 0.0, sin(territoryPatrolHeading))
        let territoryPatrolForward = SIMD3<Float>(sin(territoryPatrolHeading), 0.0, cos(territoryPatrolHeading))
        let territoryInnerRight = SIMD3<Float>(cos(territoryInnerHeading), 0.0, sin(territoryInnerHeading))
        let territoryInnerForward = SIMD3<Float>(sin(territoryInnerHeading), 0.0, cos(territoryInnerHeading))
        let hitFlashActive = (state.melee_attack_connected != 0 && state.melee_attack_phase == UInt32(MDTBMeleeAttackStrike))
            || (state.firearm_last_shot_hit != 0 && state.firearm_last_shot_timer > 0.0)

        var vertices: [Vertex] = []
        vertices.reserveCapacity(36 * 36)

        func appendHealthBar(center: SIMD3<Float>, ratio: Float, color: SIMD4<Float>) {
            let clampedRatio = max(0.0, min(ratio, 1.0))
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: center,
                halfExtents: SIMD3<Float>(0.68, 0.04, 0.05),
                yaw: 0.0,
                color: SIMD4<Float>(0.16, 0.18, 0.20, 0.72)
            ))
            if clampedRatio > 0.0 {
                vertices.append(contentsOf: makeWorldBoxVertices(
                    center: SIMD3<Float>(center.x - (0.68 * (1.0 - clampedRatio)), center.y, center.z),
                    halfExtents: SIMD3<Float>(0.68 * clampedRatio, 0.03, 0.04),
                    yaw: 0.0,
                    color: color
                ))
            }
        }

        func appendFocusMarker(position: SIMD3<Float>, alignment: Float, baseColor: SIMD4<Float>) {
            let clampedAlignment = max(0.0, min(alignment, 1.0))
            let pulse = 0.74 + (sin(elapsedTime * 8.6) * 0.18)
            let color = SIMD4<Float>(
                baseColor.x,
                min(baseColor.y + clampedAlignment * 0.28, 1.0),
                min(baseColor.z + clampedAlignment * 0.22, 1.0),
                0.78
            )

            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(position.x, position.y + 2.48, position.z),
                halfExtents: SIMD3<Float>(0.16 + clampedAlignment * 0.08, 0.05, 0.16 + clampedAlignment * 0.08),
                yaw: elapsedTime * 1.8,
                color: animatedColor(color, intensity: pulse)
            ))
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(position.x, position.y + 0.07, position.z),
                halfExtents: SIMD3<Float>(0.66 + clampedAlignment * 0.24, 0.02, 0.66 + clampedAlignment * 0.24),
                yaw: 0.0,
                color: SIMD4<Float>(color.x, color.y, color.z, 0.34)
            ))
        }

        func appendHitFlash(position: SIMD3<Float>, color: SIMD4<Float>, scale: Float) {
            let pulse = 0.70 + shotPulse * 0.40
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: position,
                halfExtents: SIMD3<Float>(0.14 + scale * pulse, 0.12 + scale * 0.4, 0.14 + scale * pulse),
                yaw: elapsedTime * 1.1,
                color: color
            ))
        }

        func appendTerritoryPatrolActor() {
            let pulse = 0.76 + (sin(elapsedTime * 5.0) * 0.14)
            let bodyColor: SIMD4<Float>
            let accentColor: SIMD4<Float>
            let ringColor: SIMD4<Float>
            let screenActive = territoryPatrolState == UInt32(MDTBTerritoryPatrolScreen)
            let clearActive = territoryPatrolState == UInt32(MDTBTerritoryPatrolClear)
            let reformActive = territoryPatrolState == UInt32(MDTBTerritoryPatrolReform)
            let braceActive = territoryPatrolState == UInt32(MDTBTerritoryPatrolBrace)

            switch territoryPatrolState {
            case UInt32(MDTBTerritoryPatrolHandoff):
                bodyColor = SIMD4<Float>(0.40 + territoryPatrolAlert * 0.20, 0.18, 0.18, 1.0)
                accentColor = SIMD4<Float>(1.0, 0.54 + territoryPatrolAlert * 0.18, 0.22, 1.0)
                ringColor = SIMD4<Float>(0.96, 0.40 + territoryPatrolAlert * 0.18, 0.18, 0.24 + territoryPatrolAlert * 0.18)
            case UInt32(MDTBTerritoryPatrolBrace):
                bodyColor = SIMD4<Float>(0.38 + territoryPatrolAlert * 0.18, 0.20, 0.16, 1.0)
                accentColor = SIMD4<Float>(1.0, 0.68 + territoryPatrolAlert * 0.14, 0.24, 1.0)
                ringColor = SIMD4<Float>(1.0, 0.54 + territoryPatrolAlert * 0.14, 0.20, 0.22 + territoryPatrolAlert * 0.18)
            case UInt32(MDTBTerritoryPatrolReform):
                bodyColor = SIMD4<Float>(0.28 + territoryPatrolAlert * 0.14, 0.22, 0.18, 1.0)
                accentColor = SIMD4<Float>(0.94, 0.72, 0.28 + territoryPatrolAlert * 0.08, 1.0)
                ringColor = SIMD4<Float>(0.78, 0.58 + territoryPatrolAlert * 0.10, 0.24, 0.16 + territoryPatrolAlert * 0.12)
            case UInt32(MDTBTerritoryPatrolClear):
                bodyColor = SIMD4<Float>(0.30 + territoryPatrolAlert * 0.14, 0.22, 0.18, 1.0)
                accentColor = SIMD4<Float>(0.92, 0.74, 0.30 + territoryPatrolAlert * 0.10, 1.0)
                ringColor = SIMD4<Float>(0.76, 0.54 + territoryPatrolAlert * 0.12, 0.22, 0.16 + territoryPatrolAlert * 0.12)
            case UInt32(MDTBTerritoryPatrolScreen):
                bodyColor = SIMD4<Float>(0.34 + territoryPatrolAlert * 0.18, 0.20, 0.18, 1.0)
                accentColor = SIMD4<Float>(1.0, 0.66 + territoryPatrolAlert * 0.14, 0.22, 1.0)
                ringColor = SIMD4<Float>(0.98, 0.48 + territoryPatrolAlert * 0.16, 0.18, 0.22 + territoryPatrolAlert * 0.18)
            case UInt32(MDTBTerritoryPatrolWatch):
                bodyColor = SIMD4<Float>(0.28 + territoryPatrolAlert * 0.16, 0.20, 0.20, 1.0)
                accentColor = SIMD4<Float>(0.94, 0.56 + territoryPatrolAlert * 0.16, 0.28, 1.0)
                ringColor = SIMD4<Float>(0.82, 0.34 + territoryPatrolAlert * 0.14, 0.20, 0.16 + territoryPatrolAlert * 0.14)
            case UInt32(MDTBTerritoryPatrolCooldown):
                bodyColor = SIMD4<Float>(0.26, 0.24, 0.28 + territoryPatrolAlert * 0.12, 1.0)
                accentColor = SIMD4<Float>(0.72, 0.62, 0.40 + territoryPatrolAlert * 0.12, 1.0)
                ringColor = SIMD4<Float>(0.56, 0.48, 0.30, 0.10 + territoryPatrolAlert * 0.10)
            default:
                bodyColor = SIMD4<Float>(0.22, 0.22, 0.24, 1.0)
                accentColor = SIMD4<Float>(0.58, 0.44, 0.30, 1.0)
                ringColor = SIMD4<Float>(0.38, 0.24, 0.16, 0.08 + territoryPatrolAlert * 0.08)
            }

            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(territoryPatrolPosition.x, territoryPatrolPosition.y + 0.05, territoryPatrolPosition.z),
                halfExtents: SIMD3<Float>(0.26 + territoryPatrolAlert * 0.12, 0.02, 0.26 + territoryPatrolAlert * 0.12),
                yaw: 0.0,
                color: ringColor
            ))
            vertices.append(contentsOf: makePedestrianPlaceholderVertices(
                position: territoryPatrolPosition,
                heading: territoryPatrolHeading,
                elapsedTime: elapsedTime + 0.12 + territoryPatrolAlert * 0.22,
                tint: bodyColor
            ))
            vertices.append(contentsOf: makeActorPartVertices(
                position: territoryPatrolPosition,
                localCenter: SIMD3<Float>(clearActive ? 0.14 : ((reformActive || braceActive) ? 0.08 : 0.0), 0.98, screenActive ? -0.24 : (braceActive ? -0.20 : (clearActive ? -0.08 : (reformActive ? -0.12 : -0.15)))),
                halfExtents: SIMD3<Float>((screenActive || braceActive) ? 0.18 : ((clearActive || reformActive) ? 0.12 : 0.14), 0.16 + ((screenActive || braceActive) ? 0.02 : 0.0), 0.05),
                bodyYaw: territoryPatrolHeading,
                partYaw: territoryPatrolHeading,
                color: accentColor
            ))
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: territoryPatrolPosition + (territoryPatrolRight * 0.24) + SIMD3<Float>(0.0, 0.92, 0.0),
                halfExtents: SIMD3<Float>(0.08, 0.06, 0.06),
                yaw: territoryPatrolHeading,
                color: animatedColor(accentColor, intensity: pulse + territoryPatrolAlert * 0.16)
            ))

            if screenActive {
                vertices.append(contentsOf: makeWorldBoxVertices(
                    center: territoryPatrolPosition + (territoryPatrolForward * 0.28) + SIMD3<Float>(0.0, 0.78, 0.0),
                    halfExtents: SIMD3<Float>(0.50 + territoryPatrolAlert * 0.18, 0.04, 0.06),
                    yaw: territoryPatrolHeading,
                    color: animatedColor(accentColor, intensity: 0.82 + territoryPatrolAlert * 0.16)
                ))
            } else if braceActive {
                vertices.append(contentsOf: makeWorldBoxVertices(
                    center: territoryPatrolPosition + (territoryPatrolForward * 0.24) + SIMD3<Float>(0.0, 0.78, 0.0),
                    halfExtents: SIMD3<Float>(0.42 + territoryPatrolAlert * 0.16, 0.04, 0.06),
                    yaw: territoryPatrolHeading,
                    color: animatedColor(accentColor, intensity: 0.84 + territoryPatrolAlert * 0.16)
                ))
            } else if reformActive {
                vertices.append(contentsOf: makeWorldBoxVertices(
                    center: territoryPatrolPosition + (territoryPatrolForward * 0.16) + SIMD3<Float>(0.0, 0.76, 0.0),
                    halfExtents: SIMD3<Float>(0.32 + territoryPatrolAlert * 0.12, 0.04, 0.05),
                    yaw: territoryPatrolHeading,
                    color: animatedColor(accentColor, intensity: 0.78 + territoryPatrolAlert * 0.12)
                ))
            } else if clearActive {
                vertices.append(contentsOf: makeWorldBoxVertices(
                    center: territoryPatrolPosition + (territoryPatrolRight * 0.34) + SIMD3<Float>(0.0, 0.74, 0.0),
                    halfExtents: SIMD3<Float>(0.10, 0.04, 0.32 + territoryPatrolAlert * 0.12),
                    yaw: territoryPatrolHeading,
                    color: animatedColor(accentColor, intensity: 0.78 + territoryPatrolAlert * 0.14)
                ))
            }

            if territoryPatrolAlert > 0.08 || territoryPatrolState == UInt32(MDTBTerritoryPatrolHandoff) {
                vertices.append(contentsOf: makeWorldBoxVertices(
                    center: SIMD3<Float>(territoryPatrolPosition.x, territoryPatrolPosition.y + 2.10, territoryPatrolPosition.z),
                    halfExtents: SIMD3<Float>(0.10 + territoryPatrolAlert * 0.08, 0.05, 0.10 + territoryPatrolAlert * 0.08),
                    yaw: elapsedTime * 1.5,
                    color: animatedColor(accentColor, intensity: 0.88 + territoryPatrolAlert * 0.18)
                ))
            }
        }

        func appendTerritoryPresenceMarker(position: SIMD3<Float>, heading: Float, right: SIMD3<Float>, forward: SIMD3<Float>, watch: Float, state: UInt32, isDeep: Bool) {
            guard territoryClaimed || watch > 0.08 else {
                return
            }

            let watchPulse = 0.76 + (sin(elapsedTime * (isDeep ? 4.8 : 5.4)) * 0.14)
            let bodyColor: SIMD4<Float>
            let accentColor: SIMD4<Float>
            let ringColor: SIMD4<Float>
            let accentCenter: SIMD3<Float>
            let accentHalfExtents: SIMD3<Float>

            switch state {
            case UInt32(MDTBTerritoryPatrolHandoff):
                bodyColor = SIMD4<Float>(0.28 + watch * 0.26 + (territoryHot ? 0.10 : 0.0), 0.16 + watch * 0.08, 0.16 + watch * 0.08, 1.0)
                accentColor = SIMD4<Float>(1.0, 0.58 + watch * 0.20, 0.20, 1.0)
                ringColor = SIMD4<Float>(0.94, 0.40 + watch * 0.22, 0.18, 0.22 + watch * 0.18 + territoryWatchTimer * 0.08)
                accentCenter = SIMD3<Float>(0.0, 0.98, isDeep ? -0.22 : -0.18)
                accentHalfExtents = SIMD3<Float>(isDeep ? 0.18 : 0.16, 0.17, 0.05)
            case UInt32(MDTBTerritoryPatrolBrace):
                bodyColor = SIMD4<Float>(0.30 + watch * 0.18, 0.18 + watch * 0.08, 0.16, 1.0)
                accentColor = SIMD4<Float>(1.0, 0.72, 0.24 + watch * 0.08, 1.0)
                ringColor = SIMD4<Float>(0.94, 0.60, 0.22, 0.18 + watch * 0.14 + territoryWatchTimer * 0.06)
                accentCenter = SIMD3<Float>(0.0, 0.98, isDeep ? -0.18 : -0.16)
                accentHalfExtents = SIMD3<Float>(isDeep ? 0.18 : 0.15, 0.16, 0.05)
            case UInt32(MDTBTerritoryPatrolReform):
                bodyColor = SIMD4<Float>(0.24 + watch * 0.16, 0.20 + watch * 0.08, 0.20, 1.0)
                accentColor = SIMD4<Float>(0.86, 0.70, 0.30 + watch * 0.08, 1.0)
                ringColor = SIMD4<Float>(0.72, 0.56, 0.24, 0.14 + watch * 0.12 + territoryWatchTimer * 0.06)
                accentCenter = SIMD3<Float>(0.0, 0.96, isDeep ? -0.14 : -0.12)
                accentHalfExtents = SIMD3<Float>(isDeep ? 0.16 : 0.12, 0.15, 0.05)
            case UInt32(MDTBTerritoryPatrolCooldown):
                bodyColor = SIMD4<Float>(0.22 + watch * 0.10, 0.18 + watch * 0.08, 0.22 + watch * 0.12, 1.0)
                accentColor = SIMD4<Float>(0.72, 0.64, 0.44 + watch * 0.08, 1.0)
                ringColor = SIMD4<Float>(0.56, 0.48, 0.30, 0.10 + watch * 0.10 + territoryWatchTimer * 0.06)
                accentCenter = SIMD3<Float>(0.0, 0.96, -0.08)
                accentHalfExtents = SIMD3<Float>(0.12, 0.14, 0.04)
            default:
                bodyColor = SIMD4<Float>(
                    0.18 + watch * 0.28 + (territoryHot ? 0.12 : 0.0),
                    0.14 + watch * 0.12,
                    0.16 + watch * 0.08,
                    1.0
                )
                accentColor = SIMD4<Float>(
                    0.94,
                    0.52 + watch * 0.22,
                    0.18 + watch * 0.10,
                    1.0
                )
                ringColor = SIMD4<Float>(
                    0.90,
                    0.34 + watch * 0.26,
                    0.18,
                    0.18 + watch * 0.16 + territoryWatchTimer * 0.10
                )
                accentCenter = SIMD3<Float>(0.0, 0.98, isDeep ? -0.10 : -0.15)
                accentHalfExtents = SIMD3<Float>(isDeep ? 0.18 : 0.14, isDeep ? 0.15 : 0.16, 0.05)
            }

            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(position.x, position.y + 0.05, position.z),
                halfExtents: SIMD3<Float>(0.26 + watch * 0.12, 0.02, 0.26 + watch * 0.12),
                yaw: 0.0,
                color: ringColor
            ))
            vertices.append(contentsOf: makePedestrianPlaceholderVertices(
                position: position,
                heading: heading,
                elapsedTime: elapsedTime + (isDeep ? 0.62 : 0.24) + watch * 0.32,
                tint: bodyColor
            ))
            vertices.append(contentsOf: makeActorPartVertices(
                position: position,
                localCenter: accentCenter,
                halfExtents: accentHalfExtents,
                bodyYaw: heading,
                partYaw: heading,
                color: accentColor
            ))
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: position + (right * 0.24) + SIMD3<Float>(0.0, 0.92, 0.0),
                halfExtents: SIMD3<Float>(0.08, 0.06, 0.06),
                yaw: heading,
                color: animatedColor(accentColor, intensity: watchPulse + watch * 0.16)
            ))

            if state == UInt32(MDTBTerritoryPatrolHandoff) ||
                state == UInt32(MDTBTerritoryPatrolBrace) ||
                state == UInt32(MDTBTerritoryPatrolReform) ||
                (isDeep && watch > 0.18) {
                vertices.append(contentsOf: makeWorldBoxVertices(
                    center: position + (forward * (isDeep ? 0.22 : 0.16)) + SIMD3<Float>(0.0, 0.82, 0.0),
                    halfExtents: SIMD3<Float>(isDeep ? 0.18 : 0.14, 0.04, 0.06),
                    yaw: heading,
                    color: animatedColor(accentColor, intensity: 0.80 + watch * 0.16)
                ))
            }

            if watch > 0.10 {
                vertices.append(contentsOf: makeWorldBoxVertices(
                    center: SIMD3<Float>(position.x, position.y + 2.14, position.z),
                    halfExtents: SIMD3<Float>(0.10 + watch * 0.08, 0.05, 0.10 + watch * 0.08),
                    yaw: elapsedTime * (isDeep ? 1.4 : 1.7),
                    color: animatedColor(accentColor, intensity: 0.88 + watch * 0.18)
                ))
            }
        }

        if state.traversal_mode == UInt32(MDTBTraversalModeOnFoot) {
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(playerPosition.x, playerPosition.y + 0.05, playerPosition.z),
                halfExtents: SIMD3<Float>(0.42 + (playerInCover ? 0.12 : 0.0), 0.03, 0.42 + (playerInCover ? 0.12 : 0.0)),
                yaw: 0.0,
                color: playerRingColor
            ))

            appendHealthBar(
                center: SIMD3<Float>(playerPosition.x, playerPosition.y + 2.38, playerPosition.z),
                ratio: playerHealthRatio,
                color: playerInCover
                    ? SIMD4<Float>(0.40, 0.82, 1.0, 0.94)
                    : SIMD4<Float>(1.0, 0.54 + playerHealthRatio * 0.20, 0.24, 0.94)
            )

            if playerDamagePulse > 0.0 {
                vertices.append(contentsOf: makeWorldBoxVertices(
                    center: playerFocus,
                    halfExtents: SIMD3<Float>(0.12 + playerDamagePulse * 0.14, 0.14 + playerDamagePulse * 0.18, 0.12 + playerDamagePulse * 0.14),
                    yaw: elapsedTime * 1.2,
                    color: SIMD4<Float>(1.0, 0.26, 0.20, 0.42 + playerDamagePulse * 0.28)
                ))
            }
        }

        if state.combat_hostile_health > 0.0 {
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(hostileAnchor.x, hostileAnchor.y + 0.04, hostileAnchor.z),
                halfExtents: SIMD3<Float>(0.42 + hostileReacquirePulse * 0.12, 0.02, 0.42 + hostileReacquirePulse * 0.12),
                yaw: 0.0,
                color: SIMD4<Float>(0.92, 0.30 + hostileReacquirePulse * 0.28, 0.18, 0.26 + hostileReacquirePulse * 0.22)
            ))

            if hostileAnchorTravel > 0.08 {
                let routeMidpoint = hostilePosition + ((hostileAnchor - hostilePosition) * 0.5)
                vertices.append(contentsOf: makeWorldBoxVertices(
                    center: SIMD3<Float>(routeMidpoint.x, routeMidpoint.y + 0.06, routeMidpoint.z),
                    halfExtents: SIMD3<Float>(0.14 + hostileAnchorTravel * 0.08, 0.02, 0.42 + hostileAnchorTravel * 0.24),
                    yaw: atan2f(hostileAnchor.x - hostilePosition.x, hostileAnchor.z - hostilePosition.z),
                    color: SIMD4<Float>(0.96, 0.44, 0.22, 0.20 + hostileAnchorTravel * 0.20)
                ))
            }
        }

        if searchActive {
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(searchPosition.x, searchPosition.y + 0.03, searchPosition.z),
                halfExtents: SIMD3<Float>(0.42 + searchCharge * 0.18 + searchPulse * 0.08, 0.02, 0.42 + searchCharge * 0.18 + searchPulse * 0.08),
                yaw: 0.0,
                color: SIMD4<Float>(0.42, 0.86, 1.0, 0.24 + searchCharge * 0.22)
            ))
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(searchPosition.x, searchPosition.y + 1.08, searchPosition.z),
                halfExtents: SIMD3<Float>(0.10 + searchCharge * 0.06, 0.34 + searchCharge * 0.12, 0.10 + searchCharge * 0.06),
                yaw: elapsedTime * 1.6,
                color: animatedColor(SIMD4<Float>(0.66, 0.94, 1.0, 0.56), intensity: searchPulse)
            ))
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(searchPosition.x, searchPosition.y + 2.04, searchPosition.z),
                halfExtents: SIMD3<Float>(0.14 + searchCharge * 0.10, 0.05, 0.14 + searchCharge * 0.10),
                yaw: elapsedTime * 2.2,
                color: animatedColor(SIMD4<Float>(0.76, 0.98, 1.0, 0.68), intensity: 0.92 + searchCharge * 0.18)
            ))
        }

        if streetIncidentActive {
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(incidentPosition.x, incidentPosition.y + 0.03, incidentPosition.z),
                halfExtents: SIMD3<Float>(0.64 + streetIncidentLevel * 0.34, 0.02, 0.64 + streetIncidentLevel * 0.34),
                yaw: 0.0,
                color: animatedColor(streetIncidentColor, intensity: incidentPulse)
            ))
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(incidentPosition.x, incidentPosition.y + 1.36, incidentPosition.z),
                halfExtents: SIMD3<Float>(0.11 + streetIncidentLevel * 0.08, 0.44 + streetIncidentLevel * 0.18, 0.11 + streetIncidentLevel * 0.08),
                yaw: elapsedTime * 1.3,
                color: animatedColor(SIMD4<Float>(1.0, 0.66, 0.26, 0.58), intensity: 0.88 + streetIncidentLevel * 0.20)
            ))

            if distanceSquared(incidentPosition, witnessPosition) > 2.0 {
                let streetRouteMidpoint = incidentPosition + ((witnessPosition - incidentPosition) * 0.5)
                vertices.append(contentsOf: makeWorldBoxVertices(
                    center: SIMD3<Float>(streetRouteMidpoint.x, streetRouteMidpoint.y + 0.05, streetRouteMidpoint.z),
                    halfExtents: SIMD3<Float>(0.12 + streetIncidentLevel * 0.06, 0.02, 0.52 + streetIncidentLevel * 0.20),
                    yaw: atan2f(witnessPosition.x - incidentPosition.x, witnessPosition.z - incidentPosition.z),
                    color: SIMD4<Float>(0.96, 0.60, 0.22, 0.16 + streetIncidentLevel * 0.12)
                ))
            }
        }

        appendTerritoryPatrolActor()
        appendTerritoryPresenceMarker(
            position: territoryInnerPosition,
            heading: territoryInnerHeading,
            right: territoryInnerRight,
            forward: territoryInnerForward,
            watch: territoryInnerVisual,
            state: territoryInnerState,
            isDeep: true
        )

        if territoryClaimed && territoryInnerVisual > 0.10 {
            let corridorMidpoint = territoryPatrolPosition + ((territoryInnerPosition - territoryPatrolPosition) * 0.5)
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(corridorMidpoint.x, corridorMidpoint.y + 0.04, corridorMidpoint.z),
                halfExtents: SIMD3<Float>(
                    (territoryPatrolBrace ? 0.16 : (territoryEntryClamp ? 0.14 : 0.10)) + territoryInnerVisual * 0.04,
                    0.02,
                    (territoryPatrolClear ? 0.48 : (territoryPatrolBrace ? 0.44 : 0.58)) + territoryInnerVisual * (territoryPatrolClear ? 0.18 : (territoryPatrolBrace ? 0.16 : 0.22))
                ),
                yaw: atan2f(territoryInnerPosition.x - territoryPatrolPosition.x, territoryInnerPosition.z - territoryPatrolPosition.z),
                color: territoryPatrolBrace
                    ? SIMD4<Float>(1.0, 0.68 + territoryInnerVisual * 0.10, 0.24, 0.20 + territoryInnerVisual * 0.12)
                    : territoryEntryClamp
                    ? SIMD4<Float>(0.96, 0.54 + territoryInnerVisual * 0.14, 0.20, 0.18 + territoryInnerVisual * 0.12)
                    : (territoryPatrolClear
                        ? SIMD4<Float>(0.82, 0.58 + territoryInnerVisual * 0.10, 0.26, 0.14 + territoryInnerVisual * 0.10)
                        : SIMD4<Float>(0.86, 0.34 + territoryInnerVisual * 0.18, 0.16, 0.12 + territoryInnerVisual * 0.10))
            ))

            if territoryEntryClamp {
                let clampMidpoint = territoryPatrolPosition + ((territoryInnerPosition - territoryPatrolPosition) * 0.32)
                vertices.append(contentsOf: makeWorldBoxVertices(
                    center: SIMD3<Float>(clampMidpoint.x, clampMidpoint.y + 0.08, clampMidpoint.z),
                    halfExtents: SIMD3<Float>(0.10 + territoryInnerVisual * 0.04, 0.03, 0.28 + territoryInnerVisual * 0.10),
                    yaw: atan2f(territoryInnerPosition.x - territoryPatrolPosition.x, territoryInnerPosition.z - territoryPatrolPosition.z),
                    color: SIMD4<Float>(1.0, 0.70, 0.24, 0.18 + territoryInnerVisual * 0.10)
                ))
            }
        }

        if territoryReclaimReturn || territoryRetakeReturn {
            let biasAnchor: SIMD3<Float>
            let biasColor: SIMD4<Float>

            if territoryReclaimFeint {
                biasAnchor = territoryReclaimCounterStep
                    ? territoryPatrolPosition + ((territoryInnerPosition - territoryPatrolPosition) * 0.32)
                    : territoryPatrolPosition + (territoryPatrolForward * 0.18)
                biasColor = territoryReclaimCounterStep
                    ? SIMD4<Float>(0.66, 1.0, 0.56, 0.36 + territoryReturnStrength * 0.18)
                    : SIMD4<Float>(0.72, 0.98, 0.48, 0.34 + territoryReturnStrength * 0.18)
            } else if territoryReclaimCommit {
                biasAnchor = territoryPatrolPosition + ((territoryInnerPosition - territoryPatrolPosition) * 0.82)
                biasColor = SIMD4<Float>(0.46, 0.98, 0.54, 0.36 + territoryReturnStrength * 0.18)
            } else if territoryRetakeChallenge {
                biasAnchor = territoryRetakeCrossAngle
                    ? territoryPatrolPosition + ((territoryInnerPosition - territoryPatrolPosition) * 0.22)
                    : territoryPatrolPosition + (territoryPatrolForward * 0.26)
                biasColor = territoryRetakeCrossAngle
                    ? SIMD4<Float>(1.0, 0.82, 0.30, 0.38 + territoryReturnStrength * 0.18)
                    : SIMD4<Float>(1.0, 0.76, 0.26, 0.36 + territoryReturnStrength * 0.18)
            } else if territoryRetakeCommit {
                biasAnchor = territoryPatrolPosition + ((territoryInnerPosition - territoryPatrolPosition) * 0.42)
                biasColor = SIMD4<Float>(0.98, 0.62, 0.22, 0.36 + territoryReturnStrength * 0.18)
            } else if territoryReclaimReturn {
                biasAnchor = territoryPatrolPosition + ((territoryInnerPosition - territoryPatrolPosition) * (territoryClaimed ? 0.76 : 0.58))
                biasColor = SIMD4<Float>(0.58, 0.98, 0.50, 0.34 + territoryReturnStrength * 0.18)
            } else {
                biasAnchor = territoryPatrolPosition + (territoryPatrolForward * 0.30)
                biasColor = SIMD4<Float>(1.0, 0.68, 0.24, 0.34 + territoryReturnStrength * 0.18)
            }

            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(biasAnchor.x, biasAnchor.y + 0.03, biasAnchor.z),
                halfExtents: SIMD3<Float>(0.28 + territoryReturnStrength * 0.12, 0.02, 0.28 + territoryReturnStrength * 0.12),
                yaw: 0.0,
                color: biasColor
            ))
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(biasAnchor.x, biasAnchor.y + 1.22, biasAnchor.z),
                halfExtents: SIMD3<Float>(0.08 + territoryReturnStrength * 0.04, 0.32 + territoryReturnStrength * 0.12, 0.08 + territoryReturnStrength * 0.04),
                yaw: elapsedTime * 1.8,
                color: animatedColor(biasColor, intensity: territoryReturnPulse)
            ))

            if territoryReclaimReturn && territoryInnerVisual > 0.08 {
                let reclaimMidpoint = territoryPatrolPosition + ((territoryInnerPosition - territoryPatrolPosition) * 0.64)
                vertices.append(contentsOf: makeWorldBoxVertices(
                    center: SIMD3<Float>(reclaimMidpoint.x, reclaimMidpoint.y + 0.05, reclaimMidpoint.z),
                    halfExtents: SIMD3<Float>(0.10 + territoryReturnStrength * 0.04, 0.02, 0.44 + territoryReturnStrength * 0.16),
                    yaw: atan2f(territoryInnerPosition.x - territoryPatrolPosition.x, territoryInnerPosition.z - territoryPatrolPosition.z),
                    color: SIMD4<Float>(0.62, 0.96, 0.44, 0.16 + territoryReturnStrength * 0.12)
                ))
                if territoryReclaimFeint {
                    vertices.append(contentsOf: makeWorldBoxVertices(
                        center: territoryPatrolPosition + (territoryPatrolForward * 0.26) + SIMD3<Float>(0.0, 0.07, 0.0),
                        halfExtents: SIMD3<Float>(0.20 + territoryReturnStrength * 0.08, 0.03, 0.10 + territoryReturnStrength * 0.04),
                        yaw: territoryPatrolHeading,
                        color: SIMD4<Float>(0.84, 1.0, 0.46, 0.18 + territoryReturnStrength * 0.10)
                    ))
                    if territoryReclaimCounterStep {
                        let counterMidpoint = territoryPatrolPosition + ((territoryInnerPosition - territoryPatrolPosition) * 0.42)
                        vertices.append(contentsOf: makeWorldBoxVertices(
                            center: SIMD3<Float>(counterMidpoint.x, counterMidpoint.y + 0.08, counterMidpoint.z),
                            halfExtents: SIMD3<Float>(0.12 + territoryReturnStrength * 0.04, 0.03, 0.34 + territoryReturnStrength * 0.14),
                            yaw: atan2f(territoryInnerPosition.x - territoryPatrolPosition.x, territoryInnerPosition.z - territoryPatrolPosition.z),
                            color: SIMD4<Float>(0.74, 1.0, 0.54, 0.22 + territoryReturnStrength * 0.12)
                        ))
                        vertices.append(contentsOf: makeWorldBoxVertices(
                            center: territoryInnerPosition - (territoryInnerForward * 0.18) + SIMD3<Float>(0.0, 0.10, 0.0),
                            halfExtents: SIMD3<Float>(0.10 + territoryReturnStrength * 0.04, 0.03, 0.20 + territoryReturnStrength * 0.08),
                            yaw: territoryInnerHeading,
                            color: SIMD4<Float>(0.68, 0.96, 0.44, 0.18 + territoryReturnStrength * 0.10)
                        ))
                    }
                }
            } else if territoryRetakeReturn {
                vertices.append(contentsOf: makeWorldBoxVertices(
                    center: territoryPatrolPosition + (territoryPatrolForward * 0.34) + SIMD3<Float>(0.0, 0.08, 0.0),
                    halfExtents: SIMD3<Float>(0.22 + territoryReturnStrength * 0.08, 0.03, 0.18 + territoryReturnStrength * 0.06),
                    yaw: territoryPatrolHeading,
                    color: SIMD4<Float>(1.0, 0.72, 0.22, 0.18 + territoryReturnStrength * 0.10)
                ))
                if territoryRetakeChallenge {
                    let challengeMidpoint = territoryPatrolPosition + ((territoryInnerPosition - territoryPatrolPosition) * 0.24)
                    vertices.append(contentsOf: makeWorldBoxVertices(
                        center: SIMD3<Float>(challengeMidpoint.x, challengeMidpoint.y + 0.06, challengeMidpoint.z),
                        halfExtents: SIMD3<Float>(0.10 + territoryReturnStrength * 0.04, 0.03, 0.30 + territoryReturnStrength * 0.12),
                        yaw: atan2f(territoryInnerPosition.x - territoryPatrolPosition.x, territoryInnerPosition.z - territoryPatrolPosition.z),
                        color: SIMD4<Float>(1.0, 0.78, 0.26, 0.20 + territoryReturnStrength * 0.10)
                    ))
                    if territoryRetakeCrossAngle {
                        let angleMidpoint = territoryPatrolPosition + ((territoryInnerPosition - territoryPatrolPosition) * 0.34)
                        vertices.append(contentsOf: makeWorldBoxVertices(
                            center: SIMD3<Float>(angleMidpoint.x, angleMidpoint.y + 0.08, angleMidpoint.z),
                            halfExtents: SIMD3<Float>(0.12 + territoryReturnStrength * 0.04, 0.03, 0.36 + territoryReturnStrength * 0.14),
                            yaw: atan2f(territoryInnerPosition.x - territoryPatrolPosition.x, territoryInnerPosition.z - territoryPatrolPosition.z),
                            color: SIMD4<Float>(1.0, 0.84, 0.30, 0.24 + territoryReturnStrength * 0.12)
                        ))
                        vertices.append(contentsOf: makeWorldBoxVertices(
                            center: territoryPatrolPosition + (territoryPatrolForward * 0.18) + SIMD3<Float>(0.0, 0.11, 0.0),
                            halfExtents: SIMD3<Float>(0.12 + territoryReturnStrength * 0.04, 0.03, 0.22 + territoryReturnStrength * 0.08),
                            yaw: territoryPatrolHeading,
                            color: SIMD4<Float>(1.0, 0.72, 0.20, 0.18 + territoryReturnStrength * 0.10)
                        ))
                    }
                }
            }
        }

        if territoryCommitWindow || territoryCommitActive || territoryCommitComplete || territoryResolveWindow || territoryResolveHold || territoryResolvePullout {
            let markerProgress = territoryResolveWindow || territoryResolveHold || territoryResolvePullout
                ? territoryResolveProgress
                : territoryCommitProgress
            let commitAnchor: SIMD3<Float>
            if territoryResolveHold {
                commitAnchor = territoryPatrolPosition + ((territoryInnerPosition - territoryPatrolPosition) * 0.82)
            } else if territoryResolvePullout {
                commitAnchor = territoryPatrolPosition - (territoryPatrolForward * 0.26)
            } else if territoryResolveWindow {
                commitAnchor = territoryPatrolPosition + ((territoryInnerPosition - territoryPatrolPosition) * 0.62)
            } else {
                commitAnchor = territoryClaimed && territoryInnerVisual > 0.10
                    ? territoryPatrolPosition + ((territoryInnerPosition - territoryPatrolPosition) * (territoryCommitComplete ? 0.74 : (territoryCommitActive ? 0.58 : 0.34)))
                    : territoryPatrolPosition + (territoryPatrolForward * (territoryCommitWindow ? 0.18 : 0.30))
            }
            let commitColor: SIMD4<Float>
            if territoryResolveHeld {
                commitColor = SIMD4<Float>(0.56, 0.98, 0.54, 0.60)
            } else if territoryResolveHold {
                commitColor = SIMD4<Float>(0.96, 0.88, 0.30, 0.58)
            } else if territoryResolveClean {
                commitColor = SIMD4<Float>(0.48, 0.92, 1.0, 0.60)
            } else if territoryResolvePullout {
                commitColor = SIMD4<Float>(0.28, 0.76, 1.0, 0.54)
            } else if territoryResolveWindow {
                commitColor = SIMD4<Float>(0.94, 0.84, 0.34, 0.50)
            } else if territoryCommitComplete {
                commitColor = SIMD4<Float>(0.76, 0.96, 0.42, 0.60)
            } else if territoryCommitActive {
                commitColor = SIMD4<Float>(1.0, 0.78, 0.26, 0.56)
            } else {
                commitColor = SIMD4<Float>(0.96, 0.62, 0.22, 0.46)
            }
            let commitPulse = 0.80 + (sin(elapsedTime * ((territoryResolveHold || territoryResolvePullout) ? 5.0 : (territoryCommitComplete ? 4.8 : 6.2))) * 0.16)

            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(commitAnchor.x, commitAnchor.y + 0.04, commitAnchor.z),
                halfExtents: SIMD3<Float>(0.20 + markerProgress * 0.16, 0.02, 0.20 + markerProgress * 0.16),
                yaw: 0.0,
                color: animatedColor(commitColor, intensity: commitPulse)
            ))
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(commitAnchor.x, commitAnchor.y + 1.20, commitAnchor.z),
                halfExtents: SIMD3<Float>(0.08 + markerProgress * 0.04, 0.30 + markerProgress * 0.12, 0.08 + markerProgress * 0.04),
                yaw: elapsedTime * 1.4,
                color: animatedColor(SIMD4<Float>(commitColor.x, min(commitColor.y + 0.14, 1.0), commitColor.z, 0.52), intensity: 0.90 + markerProgress * 0.12)
            ))
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(commitAnchor.x, commitAnchor.y + 2.02, commitAnchor.z),
                halfExtents: SIMD3<Float>(0.10 + markerProgress * 0.08, 0.05, 0.10 + markerProgress * 0.08),
                yaw: elapsedTime * 2.0,
                color: animatedColor(SIMD4<Float>(commitColor.x, min(commitColor.y + 0.18, 1.0), min(commitColor.z + 0.06, 1.0), 0.66), intensity: 0.92 + markerProgress * 0.14)
            ))
        }

        vertices.append(contentsOf: makeWorldBoxVertices(
            center: SIMD3<Float>(witnessPosition.x, witnessPosition.y + 0.05, witnessPosition.z),
            halfExtents: SIMD3<Float>(0.34 + witnessAlert * 0.14, 0.02, 0.34 + witnessAlert * 0.14),
            yaw: 0.0,
            color: witnessRingColor
        ))
        vertices.append(contentsOf: makePedestrianPlaceholderVertices(
            position: witnessPosition,
            heading: witnessHeading,
            elapsedTime: elapsedTime + Float(witnessState) * 0.18 + witnessAlert * 0.24,
            tint: witnessBodyColor
        ))
        vertices.append(contentsOf: makeActorPartVertices(
            position: witnessPosition,
            localCenter: SIMD3<Float>(0.0, 0.96, -0.16),
            halfExtents: SIMD3<Float>(0.15, 0.16, 0.05),
            bodyYaw: witnessHeading,
            partYaw: witnessHeading,
            color: witnessAccentColor
        ))
        vertices.append(contentsOf: makeWorldBoxVertices(
            center: witnessPosition + (witnessRight * 0.26) + SIMD3<Float>(0.0, 0.92, 0.0),
            halfExtents: SIMD3<Float>(0.08, 0.06, 0.06),
            yaw: witnessHeading,
            color: animatedColor(witnessAccentColor, intensity: 0.84 + witnessAlert * 0.22)
        ))

        if witnessAlert > 0.06 {
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(witnessPosition.x, witnessPosition.y + 2.16, witnessPosition.z),
                halfExtents: SIMD3<Float>(0.12 + witnessAlert * 0.08, 0.05, 0.12 + witnessAlert * 0.08),
                yaw: elapsedTime * 1.9,
                color: animatedColor(witnessAccentColor, intensity: 0.92 + witnessAlert * 0.24)
            ))
        }

        vertices.append(contentsOf: makeWorldBoxVertices(
            center: SIMD3<Float>(bystanderPosition.x, bystanderPosition.y + 0.05, bystanderPosition.z),
            halfExtents: SIMD3<Float>(0.30 + bystanderAlert * 0.14, 0.02, 0.30 + bystanderAlert * 0.14),
            yaw: 0.0,
            color: bystanderRingColor
        ))
        vertices.append(contentsOf: makePedestrianPlaceholderVertices(
            position: bystanderPosition,
            heading: bystanderHeading,
            elapsedTime: elapsedTime + 0.45 + Float(bystanderState) * 0.22 + bystanderAlert * 0.28,
            tint: bystanderBodyColor
        ))
        vertices.append(contentsOf: makeActorPartVertices(
            position: bystanderPosition,
            localCenter: SIMD3<Float>(0.0, 0.98, -0.14),
            halfExtents: SIMD3<Float>(0.14, 0.16, 0.05),
            bodyYaw: bystanderHeading,
            partYaw: bystanderHeading,
            color: bystanderAccentColor
        ))
        vertices.append(contentsOf: makeWorldBoxVertices(
            center: bystanderPosition + (bystanderRight * 0.24) + SIMD3<Float>(0.0, 0.90, 0.0),
            halfExtents: SIMD3<Float>(0.08, 0.06, 0.06),
            yaw: bystanderHeading,
            color: animatedColor(bystanderAccentColor, intensity: 0.84 + bystanderAlert * 0.22)
        ))

        if bystanderAlert > 0.06 {
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(bystanderPosition.x, bystanderPosition.y + 2.08, bystanderPosition.z),
                halfExtents: SIMD3<Float>(0.12 + bystanderAlert * 0.08, 0.05, 0.12 + bystanderAlert * 0.08),
                yaw: elapsedTime * 1.7,
                color: animatedColor(bystanderAccentColor, intensity: 0.90 + bystanderAlert * 0.22)
            ))
        }

        if state.melee_weapon_owned == 0 {
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(pipePickupPosition.x, pipePickupPosition.y + 0.09, pipePickupPosition.z),
                halfExtents: SIMD3<Float>(0.06, 0.04, 0.54),
                yaw: -0.92,
                color: pipePickupColor
            ))
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(pipePickupPosition.x, pipePickupPosition.y + 0.08, pipePickupPosition.z - 0.42),
                halfExtents: SIMD3<Float>(0.12, 0.05, 0.10),
                yaw: -0.92,
                color: SIMD4<Float>(0.16, 0.17, 0.19, 1.0)
            ))
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(pipePickupPosition.x, pipePickupPosition.y + 0.02, pipePickupPosition.z),
                halfExtents: SIMD3<Float>(0.48 + pickupPulse * 0.14, 0.02, 0.48 + pickupPulse * 0.14),
                yaw: 0.0,
                color: pipePickupHalo
            ))
        }

        if state.firearm_owned == 0 {
            vertices.append(contentsOf: artMeshes.makeVertices(
                for: .pistol,
                position: pistolPickupPosition + SIMD3<Float>(0.0, 0.14, 0.04),
                yaw: 0.34,
                roll: -.pi * 0.28,
                scale: SIMD3<Float>(repeating: 0.09),
                tint: SIMD4<Float>(0.96, 0.96, 0.96, 1.0)
            ))
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: SIMD3<Float>(pistolPickupPosition.x, pistolPickupPosition.y + 0.02, pistolPickupPosition.z),
                halfExtents: SIMD3<Float>(0.46 + pickupPulse * 0.12, 0.02, 0.46 + pickupPulse * 0.12),
                yaw: 0.0,
                color: pistolPickupHalo
            ))
        }

        if dummyDowned {
            vertices.append(contentsOf: artMeshes.makeVertices(
                for: .characterL,
                position: dummyPosition + SIMD3<Float>(0.0, 0.34, 0.0),
                yaw: 0.18,
                roll: .pi * 0.5,
                scale: SIMD3<Float>(repeating: 0.82),
                tint: blendedColor(dummyBodyColor, SIMD4<Float>(0.74, 0.68, 0.62, 1.0), amount: 0.22)
            ))
        } else {
            vertices.append(contentsOf: artMeshes.makeVertices(
                for: .characterL,
                position: dummyPosition,
                yaw: 0.0,
                scale: SIMD3<Float>(repeating: 0.82),
                tint: dummyBodyColor,
                lift: dummyReaction * 0.08
            ))
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: dummyPosition + SIMD3<Float>(0.0, 1.92 + dummyReaction * 0.10, 0.0),
                halfExtents: SIMD3<Float>(0.10, 0.08, 0.10),
                yaw: elapsedTime * 1.2,
                color: animatedColor(dummyAccentColor, intensity: 0.86 + dummyReaction * 0.18)
            ))
        }

        vertices.append(contentsOf: makeWorldBoxVertices(
            center: SIMD3<Float>(dummyPosition.x, dummyPosition.y + 0.06, dummyPosition.z),
            halfExtents: SIMD3<Float>(0.48 + (dummyInRange ? 0.18 : 0.0), 0.03, 0.48 + (dummyInRange ? 0.18 : 0.0)),
            yaw: 0.0,
            color: dummyInRange
                ? SIMD4<Float>(0.96, 0.72, 0.28, 0.52)
                : SIMD4<Float>(0.44, 0.58, 0.66, 0.32)
        ))

        appendHealthBar(
            center: SIMD3<Float>(dummyPosition.x, dummyPosition.y + 2.34, dummyPosition.z),
            ratio: dummyDowned ? 0.0 : dummyHealthRatio,
            color: SIMD4<Float>(1.0, 0.70, 0.24, 0.92)
        )

        if focusKind == UInt32(MDTBCombatTargetDummy) {
            appendFocusMarker(
                position: dummyPosition,
                alignment: state.combat_focus_alignment,
                baseColor: focusOccluded
                    ? SIMD4<Float>(0.62, 0.70, 0.76, 1.0)
                    : SIMD4<Float>(0.92, 0.68, 0.26, 1.0)
            )
        }

        if hitFlashActive && hitKind == UInt32(MDTBCombatTargetDummy) {
            appendHitFlash(
                position: SIMD3<Float>(dummyPosition.x, dummyPosition.y + 1.30, dummyPosition.z + 0.12),
                color: SIMD4<Float>(1.0, 0.78, 0.32, 0.88),
                scale: 0.16 + dummyReaction * 0.12
            )
        }

        if hostileDowned {
            vertices.append(contentsOf: artMeshes.makeVertices(
                for: .characterG,
                position: hostilePosition + SIMD3<Float>(0.0, 0.34, 0.0),
                yaw: hostileHeading + 0.38,
                roll: -.pi * 0.5,
                scale: SIMD3<Float>(repeating: 0.84),
                tint: blendedColor(hostileBodyColor, SIMD4<Float>(0.78, 0.46, 0.34, 1.0), amount: 0.18)
            ))
        } else {
            vertices.append(contentsOf: artMeshes.makeVertices(
                for: .characterG,
                position: hostilePosition,
                yaw: hostileHeading,
                scale: SIMD3<Float>(repeating: 0.84),
                tint: hostileBodyColor,
                lift: hostileReaction * 0.08
            ))
            vertices.append(contentsOf: makeHeldWeaponVertices(
                position: hostilePosition,
                heading: hostileHeading,
                lateralOffset: 0.34,
                forwardOffset: 0.24,
                heightOffset: 1.10 + hostileReaction * 0.08,
                meshID: .pistol,
                scale: SIMD3<Float>(repeating: 0.085),
                tint: SIMD4<Float>(0.96, 0.96, 0.96, 1.0)
            ))
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: hostilePosition + SIMD3<Float>(0.0, 2.02 + hostileReaction * 0.10, 0.0),
                halfExtents: SIMD3<Float>(0.10, 0.08, 0.10),
                yaw: elapsedTime * 1.3,
                color: animatedColor(hostileAccentColor, intensity: 0.88 + hostileAlert * 0.18)
            ))
        }

        vertices.append(contentsOf: makeWorldBoxVertices(
            center: SIMD3<Float>(hostilePosition.x, hostilePosition.y + 0.06, hostilePosition.z),
            halfExtents: SIMD3<Float>(0.50 + hostileAlert * 0.18 + (hostileInRange ? 0.12 : 0.0), 0.03, 0.50 + hostileAlert * 0.18 + (hostileInRange ? 0.12 : 0.0)),
            yaw: 0.0,
            color: SIMD4<Float>(0.72 + hostileAlert * 0.24, 0.20 + hostileAlert * 0.20, 0.18, 0.34 + hostileAlert * 0.22)
        ))

        appendHealthBar(
            center: SIMD3<Float>(hostilePosition.x, hostilePosition.y + 2.40, hostilePosition.z),
            ratio: hostileDowned ? 0.0 : hostileHealthRatio,
            color: SIMD4<Float>(1.0, 0.34 + hostileAlert * 0.28, 0.20, 0.96)
        )

        if focusKind == UInt32(MDTBCombatTargetLookout) {
            appendFocusMarker(
                position: hostilePosition,
                alignment: state.combat_focus_alignment,
                baseColor: focusOccluded
                    ? SIMD4<Float>(0.62, 0.74, 0.82, 1.0)
                    : SIMD4<Float>(1.0, 0.34 + hostileAlert * 0.24, 0.18, 1.0)
            )
        }

        if hitFlashActive && hitKind == UInt32(MDTBCombatTargetLookout) {
            appendHitFlash(
                position: SIMD3<Float>(hostilePosition.x, hostilePosition.y + 1.34, hostilePosition.z),
                color: SIMD4<Float>(1.0, 0.54, 0.24, 0.90),
                scale: 0.18 + hostileReaction * 0.14
            )
        }

        if state.firearm_last_shot_timer > 0.0 {
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: shotFrom,
                halfExtents: SIMD3<Float>(0.12 + shotPulse * 0.08, 0.12 + shotPulse * 0.08, 0.12 + shotPulse * 0.08),
                yaw: 0.0,
                color: SIMD4<Float>(1.0, 0.82, 0.34, 0.72)
            ))
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: shotTo,
                halfExtents: SIMD3<Float>(0.10 + shotPulse * 0.12, 0.10 + shotPulse * 0.12, 0.10 + shotPulse * 0.12),
                yaw: 0.0,
                color: state.firearm_last_shot_hit != 0
                    ? SIMD4<Float>(1.0, 0.58, 0.26, 0.84)
                    : (state.firearm_last_shot_blocked != 0
                        ? SIMD4<Float>(0.54, 0.80, 0.92, 0.72)
                        : SIMD4<Float>(0.92, 0.88, 0.68, 0.44))
            ))
        }

        if state.combat_hostile_attack_windup > 0.0 {
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: hostileShotFrom,
                halfExtents: SIMD3<Float>(0.10 + hostileAttackCharge * 0.12, 0.10 + hostileAttackCharge * 0.12, 0.10 + hostileAttackCharge * 0.12),
                yaw: elapsedTime * 2.2,
                color: SIMD4<Float>(1.0, 0.42 + hostileAttackCharge * 0.24, 0.18, 0.52 + hostileAttackCharge * 0.24)
            ))
        }

        if state.combat_hostile_last_shot_timer > 0.0 {
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: hostileShotFrom,
                halfExtents: SIMD3<Float>(0.10 + hostileShotPulse * 0.10, 0.10 + hostileShotPulse * 0.10, 0.10 + hostileShotPulse * 0.10),
                yaw: 0.0,
                color: SIMD4<Float>(1.0, 0.42, 0.20, 0.76)
            ))
            vertices.append(contentsOf: makeWorldBoxVertices(
                center: hostileShotTo,
                halfExtents: SIMD3<Float>(0.10 + hostileShotPulse * 0.12, 0.10 + hostileShotPulse * 0.12, 0.10 + hostileShotPulse * 0.12),
                yaw: 0.0,
                color: state.combat_hostile_last_shot_hit != 0
                    ? SIMD4<Float>(1.0, 0.28, 0.20, 0.84)
                    : SIMD4<Float>(0.40, 0.84, 0.96, 0.76)
            ))
        }

        return vertices
    }

    private static func makeActorVertices(position: SIMD3<Float>, heading: Float, cameraYaw: Float, speed: Float, elapsedTime: Float, equippedWeapon: UInt32, meleeAttackPhase: UInt32, meleeAttackTimer: Float, firearmReloading: Bool, firearmReloadTimer: Float, firearmShotTimer: Float) -> [Vertex] {
        let headDelta = max(min(Self.wrapAngle(cameraYaw - heading), 0.75), -0.75)
        let isPipeEquipped = equippedWeapon == UInt32(MDTBEquippedWeaponLeadPipe)
        let isPistolEquipped = equippedWeapon == UInt32(MDTBEquippedWeaponPistol)
        let bodyYaw = heading + (headDelta * 0.24)
        let lift = sin(elapsedTime * (4.8 + min(speed * 0.22, 1.4))) * min(speed / 10.0, 0.06)
        let bodyTint = SIMD4<Float>(0.98, 0.96, 0.92, 1.0)
        var vertices = artMeshes.makeVertices(
            for: .characterA,
            position: position,
            yaw: bodyYaw,
            scale: SIMD3<Float>(repeating: 0.88),
            tint: bodyTint,
            lift: lift
        )

        if isPipeEquipped {
            let attackLift: Float = meleeAttackPhase == UInt32(MDTBMeleeAttackStrike) ? 0.16 : 0.08
            vertices.append(
                contentsOf: makeWorldBoxVertices(
                    center: position + SIMD3<Float>(cos(bodyYaw) * 0.42, 1.14 + attackLift, sin(bodyYaw) * 0.42),
                    halfExtents: SIMD3<Float>(0.05, 0.05, 0.72),
                    yaw: bodyYaw,
                    color: SIMD4<Float>(0.62, 0.64, 0.69, 1.0)
                )
            )
        } else if isPistolEquipped {
            let recoil = min(max(firearmShotTimer / 0.10, 0.0), 1.0)
            let reloadLift = firearmReloading ? 1.0 - min(max(firearmReloadTimer / 1.15, 0.0), 1.0) : 0.0
            vertices.append(contentsOf: makeHeldWeaponVertices(
                position: position,
                heading: bodyYaw,
                lateralOffset: 0.36,
                forwardOffset: 0.26 + reloadLift * 0.08 - recoil * 0.04,
                heightOffset: 1.18 + reloadLift * 0.12,
                meshID: .pistol,
                scale: SIMD3<Float>(repeating: 0.086),
                tint: firearmReloading ? SIMD4<Float>(1.0, 0.94, 0.78, 1.0) : SIMD4<Float>(0.96, 0.96, 0.96, 1.0)
            ))
        }

        return vertices
    }

    private static func pedestrianMeshVariant(position: SIMD3<Float>) -> ArtMeshID {
        let seed = Int(abs((position.x * 0.31) + (position.z * 0.17)).rounded())
        switch seed % 4 {
        case 0:
            return .characterA
        case 1:
            return .characterG
        case 2:
            return .characterL
        default:
            return .characterR
        }
    }

    private static func vehicleMeshDescriptor(for kind: UInt32) -> (ArtMeshID, SIMD3<Float>) {
        switch kind {
        case UInt32(MDTBVehicleKindCoupe):
            return (.hatchback, SIMD3<Float>(repeating: 1.46))
        case UInt32(MDTBVehicleKindMoped):
            return (.kart, SIMD3<Float>(repeating: 1.38))
        case UInt32(MDTBVehicleKindBicycle):
            return (.kart, SIMD3<Float>(repeating: 1.18))
        case UInt32(MDTBVehicleKindMotorcycle):
            return (.raceCar, SIMD3<Float>(repeating: 1.22))
        default:
            return (.sedan, SIMD3<Float>(repeating: 1.42))
        }
    }

    private static func makeHeldWeaponVertices(position: SIMD3<Float>, heading: Float, lateralOffset: Float, forwardOffset: Float, heightOffset: Float, meshID: ArtMeshID, scale: SIMD3<Float>, tint: SIMD4<Float> = SIMD4<Float>(repeating: 1.0)) -> [Vertex] {
        let right = SIMD3<Float>(cos(heading), 0.0, sin(heading))
        let forward = SIMD3<Float>(sin(heading), 0.0, -cos(heading))
        let weaponPosition =
            position +
            (right * lateralOffset) +
            (forward * forwardOffset) +
            SIMD3<Float>(0.0, heightOffset, 0.0)
        return artMeshes.makeVertices(for: meshID, position: weaponPosition, yaw: heading + (.pi * 0.5), scale: scale, tint: tint)
    }

    private static func environmentTint(for block: SceneBlock) -> SIMD4<Float> {
        switch block.district {
        case UInt32(MDTBDistrictKoreatown), UInt32(MDTBDistrictSouthPark), UInt32(MDTBDistrictUniversityPark):
            return SIMD4<Float>(1.0, 0.98, 0.98, 1.0)
        case UInt32(MDTBDistrictInglewood), UInt32(MDTBDistrictLeimertPark), UInt32(MDTBDistrictWillowbrook):
            return SIMD4<Float>(0.94, 1.0, 0.95, 1.0)
        case UInt32(MDTBDistrictHistoricSouthCentral), UInt32(MDTBDistrictWatts), UInt32(MDTBDistrictHuntingtonPark):
            return SIMD4<Float>(0.96, 0.94, 0.92, 1.0)
        default:
            return SIMD4<Float>(1.0, 1.0, 1.0, 1.0)
        }
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
            pedestrianDensity: (block.kind == UInt32(MDTBBlockKindResidential) || block.district == UInt32(MDTBDistrictLeimertPark) || block.district == UInt32(MDTBDistrictInglewood) || block.district == UInt32(MDTBDistrictWillowbrook))
                ? 0.56
                : ((block.district == UInt32(MDTBDistrictKoreatown) || block.district == UInt32(MDTBDistrictPicoUnion) || block.district == UInt32(MDTBDistrictUniversityPark) || block.district == UInt32(MDTBDistrictSouthPark))
                    ? 0.78
                    : 0.72),
            vehicleDensity: ((block.tagMask & UInt32(MDTBBlockTagSpur)) != 0 || block.district == UInt32(MDTBDistrictHistoricSouthCentral) || block.district == UInt32(MDTBDistrictHuntingtonPark) || block.district == UInt32(MDTBDistrictWatts))
                ? 0.82
                : (block.kind == UInt32(MDTBBlockKindResidential) ? 0.42 : 0.64),
            ambientEnergy: (block.district == UInt32(MDTBDistrictFlorenceFirestone) || block.district == UInt32(MDTBDistrictHuntingtonPark) || block.district == UInt32(MDTBDistrictWatts))
                ? 0.72
                : ((block.kind == UInt32(MDTBBlockKindResidential) || block.district == UInt32(MDTBDistrictLeimertPark) || block.district == UInt32(MDTBDistrictInglewood) || block.district == UInt32(MDTBDistrictWillowbrook))
                    ? 0.50
                    : 0.78),
            travelBias: (block.tagMask & UInt32(MDTBBlockTagSpur)) != 0 ? 0.82 : (block.district == UInt32(MDTBDistrictSouthPark) ? 0.64 : 0.56),
            styleFlags: styleFlags
        )
    }

    private static func districtAmbientColor(for block: SceneBlock) -> SIMD4<Float> {
        switch block.district {
        case UInt32(MDTBDistrictWestAdams), UInt32(MDTBDistrictJeffersonPark), UInt32(MDTBDistrictPicoUnion):
            return SIMD4<Float>(0.86, 0.58, 0.24, 1.0)
        case UInt32(MDTBDistrictExpositionPark), UInt32(MDTBDistrictLeimertPark), UInt32(MDTBDistrictInglewood):
            return SIMD4<Float>(0.46, 0.68, 0.38, 1.0)
        case UInt32(MDTBDistrictCrenshawCorridor), UInt32(MDTBDistrictUniversityPark):
            return SIMD4<Float>(0.30, 0.66, 0.78, 1.0)
        case UInt32(MDTBDistrictVermontSquare), UInt32(MDTBDistrictKoreatown):
            return SIMD4<Float>(0.40, 0.56, 0.82, 1.0)
        case UInt32(MDTBDistrictHistoricSouthCentral), UInt32(MDTBDistrictSouthPark):
            return SIMD4<Float>(0.80, 0.50, 0.28, 1.0)
        case UInt32(MDTBDistrictFlorenceFirestone), UInt32(MDTBDistrictHuntingtonPark), UInt32(MDTBDistrictWatts), UInt32(MDTBDistrictWillowbrook):
            return SIMD4<Float>(0.68, 0.46, 0.34, 1.0)
        default:
            return SIMD4<Float>(0.78, 0.42, 0.26, 1.0)
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

    private static func districtCount(_ blocks: [SceneBlock]) -> Int {
        Set(blocks.map { Int($0.district) }).count
    }

    private static func corridorCount(_ roadLinks: [SceneRoadLink]) -> Int {
        Set(roadLinks.map { Int($0.corridor) }).count
    }

    private static func mapStatus(state: MDTBEngineState, blocks: [SceneBlock], roadLinks: [SceneRoadLink]) -> (title: String, detail: String) {
        let district = state.active_block_index < blocks.count ? districtLabel(blocks[Int(state.active_block_index)].district) : "Outside map coverage"
        let chunk = activeChunkLabel(state: state, blocks: blocks)
        let districtCoverage = districtCount(blocks)
        let corridorCoverage = corridorCount(roadLinks)

        if state.active_link_index < roadLinks.count {
            let link = roadLinks[Int(state.active_link_index)]
            return (
                "\(district) / \(corridorLabel(link.corridor))",
                "\(roadClassLabel(link.roadClass)) / \(chunk) / \(districtCoverage) districts / \(corridorCoverage) named roads"
            )
        }

        return (
            district,
            "\(chunk) / \(districtCoverage) districts / \(corridorCoverage) named roads"
        )
    }

    private static func activitySummary(state: MDTBEngineState, blocks: [SceneBlock], roadLinks: [SceneRoadLink], interestPoints: [SceneInterestPoint], populationProfiles: [ScenePopulationProfile], trafficOccupancies: [SceneTrafficOccupancy], vehicleAnchors: [SceneVehicleAnchor]) -> String {
        let blockLabel = blockName(for: state.active_block_index, blocks: blocks)
        let linkLabel = linkSummary(for: state.active_link_index, roadLinks: roadLinks, blocks: blocks)
        let population = populationActivity(state: state, blocks: blocks, roadLinks: roadLinks, interestPoints: interestPoints, populationProfiles: populationProfiles, trafficOccupancies: trafficOccupancies)
        let chunkLabelValue = activeChunkLabel(state: state, blocks: blocks)
        let visibleBlocks = visibleBlockCount(state: state, blocks: blocks, roadLinks: roadLinks)
        let hotspots = hotspotCount(state: state, blocks: blocks, roadLinks: roadLinks, interestPoints: interestPoints)
        let stagedVehicles = stagedVehicleCount(state: state, blocks: blocks, roadLinks: roadLinks, vehicleAnchors: vehicleAnchors)
        return "\(blockLabel) / \(chunkLabelValue) / \(visibleBlocks) local / \(districtCount(blocks)) districts / \(corridorCount(roadLinks)) named roads / \(hotspots) hot / \(stagedVehicles) staged / \(trafficOccupancies.count) occ / \(population.livePedestrians) live ped / \(population.reactingPedestrians) reacting / \(population.liveVehicles) live veh / \(population.yieldingVehicles) easing / \(linkLabel)"
    }

    private static func blockSummary(state: MDTBEngineState, blocks: [SceneBlock], populationProfiles: [ScenePopulationProfile]) -> String {
        guard state.active_block_index < blocks.count else {
            return "none"
        }

        let blockIndex = Int(state.active_block_index)
        let block = blocks[blockIndex]
        let profile = populationProfile(for: blockIndex, block: block, populationProfiles: populationProfiles)
        let territoryTail = territoryStatusSummary(state: state).map { " / \($0)" } ?? ""
        return "\(districtLabel(block.district)) / \(blockKindLabel(block.kind)) / \(tagLabel(block.tagMask)) / \(frontageTemplateLabel(block.frontageTemplate)) / \(chunkLabel(block.chunkIndex)) / \(profileSummary(profile))\(territoryTail) / r \(formatScalar(block.activationRadius))"
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

    private static func currentPlayerPosition(state: MDTBEngineState) -> SIMD3<Float> {
        if state.traversal_mode == UInt32(MDTBTraversalModeVehicle) {
            return SIMD3<Float>(state.active_vehicle_position.x, state.active_vehicle_position.y, state.active_vehicle_position.z)
        }

        return SIMD3<Float>(state.actor_position.x, state.actor_position.y, state.actor_position.z)
    }

    private static func candidateDistance(state: MDTBEngineState, anchor: SceneVehicleAnchor) -> Float {
        sqrt(distanceSquared(currentPlayerPosition(state: state), anchor.position))
    }

    private static func rankedVehicleIndices(state: MDTBEngineState, vehicleAnchors: [SceneVehicleAnchor]) -> [Int] {
        [state.nearby_vehicle_anchor_index, state.secondary_vehicle_anchor_index, state.tertiary_vehicle_anchor_index].reduce(into: [Int]()) { indices, rawIndex in
            guard rawIndex < vehicleAnchors.count else {
                return
            }

            let index = Int(rawIndex)
            if !indices.contains(index) {
                indices.append(index)
            }
        }
    }

    private static func trafficHazardIntensity(samplePosition: SIMD3<Float>, playerPosition: SIMD3<Float>, traversalMode: UInt32) -> Float {
        let hazardRadius: Float = traversalMode == UInt32(MDTBTraversalModeVehicle) ? 7.5 : 5.8
        let distance = sqrt(distanceSquared(samplePosition, playerPosition))
        guard distance < hazardRadius else {
            return 0.0
        }

        return 1.0 - max(0.0, min(distance / hazardRadius, 1.0))
    }

    private static func incidentReactionIntensity(position: SIMD3<Float>, state: MDTBEngineState) -> Float {
        let incidentLevel = min(max(state.street_incident_level, 0.0), 1.0)
        guard state.street_incident_timer > 0.0, incidentLevel > 0.08 else {
            return 0.0
        }

        let incidentPosition = SIMD3<Float>(
            state.street_incident_position.x,
            state.street_incident_position.y,
            state.street_incident_position.z
        )
        let reactionRadius: Float = 10.0 + incidentLevel * 24.0
        let distance = sqrt(distanceSquared(position, incidentPosition))
        guard distance < reactionRadius else {
            return 0.0
        }

        let civilianBoost: Float =
            (state.witness_state != UInt32(MDTBWitnessStateIdle) ? 0.10 : 0.0) +
            (state.bystander_state != UInt32(MDTBWitnessStateIdle) ? 0.12 : 0.0)
        return min(max((1.0 - (distance / reactionRadius)) * (incidentLevel + civilianBoost), 0.0), 1.0)
    }

    private static func streetNormalizingIntensity(state: MDTBEngineState) -> Float {
        let civiliansAgitated =
            state.witness_state == UInt32(MDTBWitnessStateInvestigate) ||
            state.witness_state == UInt32(MDTBWitnessStateFlee) ||
            state.bystander_state == UInt32(MDTBWitnessStateInvestigate) ||
            state.bystander_state == UInt32(MDTBWitnessStateFlee)
        let searchActive =
            state.combat_hostile_search_timer > 0.0 ||
            state.combat_hostile_attack_windup > 0.0 ||
            state.combat_hostile_last_shot_timer > 0.0 ||
            state.firearm_last_shot_timer > 0.0

        guard state.street_incident_timer > 0.0,
              state.street_incident_level > 0.08,
              !searchActive,
              !civiliansAgitated else {
            return 0.0
        }

        return min(max(state.street_incident_level, 0.0), 1.0)
    }

    private static func streetNormalizingInfluence(position: SIMD3<Float>, state: MDTBEngineState) -> Float {
        let normalizeIntensity = streetNormalizingIntensity(state: state)
        guard normalizeIntensity > 0.0 else {
            return 0.0
        }

        return min(incidentReactionIntensity(position: position, state: state) * (0.72 + normalizeIntensity * 0.24), 1.0)
    }

    private static func streetRecoveryIntensity(state: MDTBEngineState) -> Float {
        let recoveryLevel = min(max(state.street_recovery_level, 0.0), 1.0)
        guard state.street_incident_timer <= 0.0,
              state.street_recovery_timer > 0.0,
              recoveryLevel > 0.04 else {
            return 0.0
        }

        return min(recoveryLevel * 0.88 + min(max(state.street_recovery_timer / 5.0, 0.0), 0.22), 1.0)
    }

    private static func streetRecoveryInfluence(position: SIMD3<Float>, state: MDTBEngineState) -> Float {
        let recoveryIntensity = streetRecoveryIntensity(state: state)
        guard recoveryIntensity > 0.0 else {
            return 0.0
        }

        let recoveryPosition = SIMD3<Float>(
            state.street_recovery_position.x,
            state.street_recovery_position.y,
            state.street_recovery_position.z
        )
        let recoveryRadius: Float = 8.0 + recoveryIntensity * 18.0
        let distance = sqrt(distanceSquared(position, recoveryPosition))
        guard distance < recoveryRadius else {
            return 0.0
        }

        let civilianBoost: Float = Float(civilianResponseSourceCount(state: state)) * 0.04
        return min(max((1.0 - (distance / recoveryRadius)) * (recoveryIntensity + civilianBoost), 0.0), 1.0)
    }

    private static func streetCoolInfluence(position: SIMD3<Float>, state: MDTBEngineState) -> Float {
        max(
            streetNormalizingInfluence(position: position, state: state),
            streetRecoveryInfluence(position: position, state: state)
        )
    }

    private static func roadLinkIncidentInfluence(link: SceneRoadLink, blocks: [SceneBlock], state: MDTBEngineState) -> Float {
        guard Int(link.fromBlockIndex) < blocks.count, Int(link.toBlockIndex) < blocks.count else {
            return 0.0
        }

        let midpoint = link.midpoint
        let civilianBoost = Float(civilianResponseSourceCount(state: state)) * 0.06
        return min(incidentReactionIntensity(position: midpoint, state: state) + civilianBoost, 1.0)
    }

    private static func distanceSquared(_ lhs: SIMD3<Float>, _ rhs: SIMD3<Float>) -> Float {
        let dx = lhs.x - rhs.x
        let dz = lhs.z - rhs.z
        return (dx * dx) + (dz * dz)
    }

    private static func populationActivity(state: MDTBEngineState, blocks: [SceneBlock], roadLinks: [SceneRoadLink], interestPoints: [SceneInterestPoint], populationProfiles: [ScenePopulationProfile], trafficOccupancies: [SceneTrafficOccupancy]) -> PopulationActivity {
        guard !interestPoints.isEmpty else {
            return PopulationActivity(livePedestrians: 0, reactingPedestrians: 0, liveVehicles: 0, yieldingVehicles: 0)
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
        let playerPosition = currentPlayerPosition(state: state)

        var pedestrians = 0
        var reactingPedestrians = 0
        var vehicles = 0
        var yieldingVehicles = 0

        for (pointIndex, point) in interestPoints.enumerated() {
            let blockIndex = Int(point.blockIndex)
            guard blockIndex < blocks.count, visibleBlocks.contains(blockIndex) else {
                continue
            }

            let block = blocks[blockIndex]
            let profile = populationProfile(for: blockIndex, block: block, populationProfiles: populationProfiles)
            switch point.kind {
            case UInt32(MDTBInterestPointPedestrianSpawn):
                if let sample = pedestrianSample(for: point, pointIndex: pointIndex, state: state, block: block, profile: profile, blocks: blocks, roadLinks: roadLinks, interestPoints: interestPoints, elapsedTime: state.elapsed_time) {
                    pedestrians += 1
                    if sample.reactionIntensity > 0.10 {
                        reactingPedestrians += 1
                    }
                }
            case UInt32(MDTBInterestPointVehicleSpawn):
                if let sample = vehicleSample(for: point, pointIndex: pointIndex, block: block, profile: profile, blocks: blocks, roadLinks: roadLinks, trafficOccupancies: trafficOccupancies, playerPosition: playerPosition, traversalMode: state.traversal_mode, elapsedTime: state.elapsed_time) {
                    vehicles += 1
                    if sample.yieldIntensity > 0.35 {
                        yieldingVehicles += 1
                    }
                }
            default:
                break
            }
        }

        return PopulationActivity(
            livePedestrians: pedestrians,
            reactingPedestrians: reactingPedestrians,
            liveVehicles: vehicles,
            yieldingVehicles: yieldingVehicles
        )
    }

    private static func formatScalar(_ value: Float) -> String {
        String(format: "%.1f", value)
    }

    private static func blockName(for index: UInt32, blocks: [SceneBlock]) -> String {
        guard index < blocks.count else {
            return "none"
        }

        let block = blocks[Int(index)]
        return "\(districtLabel(block.district)) \(blockKindLabel(block.kind))"
    }

    private static func linkSummary(for index: UInt32, roadLinks: [SceneRoadLink], blocks: [SceneBlock]) -> String {
        guard index < roadLinks.count else {
            return "no link"
        }

        let link = roadLinks[Int(index)]
        let axis = link.axis == UInt32(MDTBRoadAxisNorthSouth) ? "north-south" : "east-west"
        return "\(corridorLabel(link.corridor)) / \(roadClassLabel(link.roadClass)) / \(axis)"
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
            let recovery = formatScalar(state.active_vehicle_recovery)
            let steer = formatScalar(state.active_vehicle_steer_visual)
            return "\(vehicleLabel) active / \(blockLabel) / \(formatScalar(state.active_vehicle_speed)) m/s / grip \(surfaceGrip) / lane \(laneError) / bump \(bump) / recover \(recovery) / lean \(steer)"
        }

        guard state.nearby_vehicle_anchor_index < vehicleAnchors.count else {
            return "on-foot / no staged vehicle nearby"
        }

        let anchor = vehicleAnchors[Int(state.nearby_vehicle_anchor_index)]
        let distance = sqrt(distanceSquared(
            SIMD3<Float>(state.actor_position.x, state.actor_position.y, state.actor_position.z),
            anchor.position
        ))
        return "\(vehicleKindLabel(anchor.kind)) staged / \(formatScalar(distance))m / \(parkingStateLabel(anchor.parkingState)) / \(blockName(for: anchor.blockIndex, blocks: blocks)) / \(vehicleActionLabel(anchor.kind))"
    }

    private static func combatSummary(state: MDTBEngineState) -> String {
        let playerPosition = currentPlayerPosition(state: state)
        let pipePickupPosition = SIMD3<Float>(state.melee_weapon_pickup_position.x, state.melee_weapon_pickup_position.y, state.melee_weapon_pickup_position.z)
        let pistolPickupPosition = SIMD3<Float>(state.firearm_pickup_position.x, state.firearm_pickup_position.y, state.firearm_pickup_position.z)
        let dummyPosition = SIMD3<Float>(state.combat_target_position.x, state.combat_target_position.y, state.combat_target_position.z)
        let lookoutPosition = SIMD3<Float>(state.combat_hostile_position.x, state.combat_hostile_position.y, state.combat_hostile_position.z)
        let pipeDistance = formatScalar(sqrt(distanceSquared(playerPosition, pipePickupPosition)))
        let pistolDistance = formatScalar(sqrt(distanceSquared(playerPosition, pistolPickupPosition)))
        let dummyDistance = formatScalar(sqrt(distanceSquared(playerPosition, dummyPosition)))
        let lookoutDistance = formatScalar(sqrt(distanceSquared(playerPosition, lookoutPosition)))
        let playerSummary = playerStatusSummary(state: state)
        let dummySummary = dummyStatusSummary(state: state, targetDistance: dummyDistance)
        let lookoutSummary = lookoutStatusSummary(state: state, targetDistance: lookoutDistance)
        let focusSummary = combatFocusSummary(state: state)
        let systemicSummary = systemicPressureSummary(state: state)
        let laneSummary = [playerSummary, focusSummary, dummySummary, lookoutSummary, systemicSummary].compactMap { $0 }.joined(separator: " / ")
        let pistolSummary = pistolAmmoSummary(state: state)

        if state.traversal_mode == UInt32(MDTBTraversalModeVehicle) {
            guard state.equipped_weapon_kind != UInt32(MDTBEquippedWeaponNone) else {
                return "combat stowed / \(laneSummary) / \(vehicleKindLabel(state.active_vehicle_kind))"
            }

            if state.equipped_weapon_kind == UInt32(MDTBEquippedWeaponPistol) {
                return "\(equippedWeaponLabel(state.equipped_weapon_kind)) stowed / \(pistolSummary) / \(laneSummary) / \(vehicleKindLabel(state.active_vehicle_kind))"
            }

            return "\(equippedWeaponLabel(state.equipped_weapon_kind)) stowed / \(laneSummary) / \(vehicleKindLabel(state.active_vehicle_kind))"
        }

        switch state.equipped_weapon_kind {
        case UInt32(MDTBEquippedWeaponLeadPipe):
            let attackState = attackPhaseLabel(state.melee_attack_phase)
            let attackSummary = state.melee_attack_phase == UInt32(MDTBMeleeAttackIdle)
                ? "ready"
                : "\(attackState) \(formatScalar(state.melee_attack_timer))s" + (state.melee_attack_connected != 0 ? " hit" : "")
            let pistolTail = state.firearm_owned != 0 ? " / slot 2 \(pistolSummary)" : ""
            return "slot 1 lead pipe / \(attackSummary) / \(laneSummary)\(pistolTail)"
        case UInt32(MDTBEquippedWeaponPistol):
            let firearmState = state.firearm_reloading != 0
                ? "reload \(formatScalar(state.firearm_reload_timer))s"
                : (state.firearm_last_shot_timer > 0.0
                    ? (state.firearm_last_shot_hit != 0
                        ? "fired hit"
                        : (state.firearm_last_shot_blocked != 0 ? "fired cover" : "fired"))
                    : "ready")
            return "slot 2 pistol / \(pistolSummary) / \(firearmState) / \(laneSummary)"
        default:
            break
        }

        var pickupParts: [String] = []
        if state.melee_weapon_owned == 0 {
            pickupParts.append("pipe \(state.melee_weapon_pickup_in_range != 0 ? "ready" : "\(pipeDistance)m")")
        }
        if state.firearm_owned == 0 {
            pickupParts.append("pistol \(state.firearm_pickup_in_range != 0 ? "ready" : "\(pistolDistance)m")")
        }

        if pickupParts.isEmpty {
            return "no weapon equipped / \(laneSummary)"
        }

        return "pickups " + pickupParts.joined(separator: " / ") + " / " + laneSummary
    }

    private static func interactionSummary(state: MDTBEngineState, vehicleAnchors: [SceneVehicleAnchor], blocks: [SceneBlock]) -> String {
        let combatPrompt = combatInteractionPrompt(state: state)

        if state.traversal_mode == UInt32(MDTBTraversalModeVehicle) {
            if abs(state.active_vehicle_speed) <= 1.4 {
                let vehiclePrompt = "press F to \(vehicleExitLabel(state.active_vehicle_kind)) \(vehicleKindLabel(state.active_vehicle_kind))"
                return combatPrompt.map { "\($0) / \(vehiclePrompt)" } ?? vehiclePrompt
            }
            let vehiclePrompt = "slow down to \(vehicleExitLabel(state.active_vehicle_kind)) \(vehicleKindLabel(state.active_vehicle_kind))"
            return combatPrompt.map { "\($0) / \(vehiclePrompt)" } ?? vehiclePrompt
        }

        guard state.nearby_vehicle_anchor_index < vehicleAnchors.count else {
            return combatPrompt ?? "walk up to a staged vehicle and press F"
        }

        let anchor = vehicleAnchors[Int(state.nearby_vehicle_anchor_index)]
        let distance = candidateDistance(state: state, anchor: anchor)
        let ranked = rankedVehicleIndices(state: state, vehicleAnchors: vehicleAnchors)
        let nextChoice = ranked.dropFirst().first.flatMap { nextIndex -> String? in
            guard nextIndex < vehicleAnchors.count else {
                return nil
            }

            let nextAnchor = vehicleAnchors[nextIndex]
            return " / next \(vehicleKindLabel(nextAnchor.kind)) \(formatScalar(candidateDistance(state: state, anchor: nextAnchor)))m"
        } ?? ""
        let lockHint = state.vehicle_selection_locked != 0 ? "G unlock" : "G lock"
        let selectionControls = ranked.count > 1 ? " / R cycle / \(lockHint)" : " / \(lockHint)"
        let vehiclePrompt: String

        if distance <= Self.vehicleMountRadius {
            vehiclePrompt = "press F to \(vehicleActionLabel(anchor.kind)) \(vehicleKindLabel(anchor.kind)) from \(parkingStateLabel(anchor.parkingState)) \(blockName(for: anchor.blockIndex, blocks: blocks))\(nextChoice)\(selectionControls)"
        } else {
            vehiclePrompt = "approach \(vehicleKindLabel(anchor.kind)) \(formatScalar(distance))m to \(vehicleActionLabel(anchor.kind))\(nextChoice)\(selectionControls)"
        }

        return combatPrompt.map { "\($0) / \(vehiclePrompt)" } ?? vehiclePrompt
    }

    private static func handoffSelectionSummary(state: MDTBEngineState, vehicleAnchors: [SceneVehicleAnchor], blocks: [SceneBlock]) -> String {
        let ranked = rankedVehicleIndices(state: state, vehicleAnchors: vehicleAnchors)
        guard !ranked.isEmpty else {
            return state.traversal_mode == UInt32(MDTBTraversalModeVehicle) ? "active vehicle locked" : "none in preview radius"
        }

        let lockedIndex = state.vehicle_selection_locked != 0 && state.locked_vehicle_anchor_index < vehicleAnchors.count
            ? Int(state.locked_vehicle_anchor_index)
            : nil
        let summary = ranked.enumerated().map { rank, index in
            let anchor = vehicleAnchors[index]
            let distance = formatScalar(candidateDistance(state: state, anchor: anchor))
            let tags = [
                index == Int(state.nearby_vehicle_anchor_index) ? "selected" : nil,
                lockedIndex == index ? "locked" : nil,
            ].compactMap { $0 }
            let tagSummary = tags.isEmpty ? "" : " [" + tags.joined(separator: ",") + "]"
            return "\(rank + 1) \(vehicleKindLabel(anchor.kind)) \(distance)m \(parkingStateLabel(anchor.parkingState))\(tagSummary)"
        }.joined(separator: " / ")
        return "\(lockedIndex == nil ? "free" : "locked") / \(summary)"
    }

    private static func trafficHazardSummary(state: MDTBEngineState, trafficHazard: Float) -> String {
        if state.traversal_mode == UInt32(MDTBTraversalModeVehicle) {
            if trafficHazard >= 0.7 {
                return "near-miss pressure high \(formatScalar(trafficHazard))"
            }
            if trafficHazard >= 0.35 {
                return "watch traffic \(formatScalar(trafficHazard))"
            }
            return "lane pressure calm \(formatScalar(trafficHazard))"
        }

        if trafficHazard >= 0.7 {
            return "crossing hazard high \(formatScalar(trafficHazard))"
        }
        if trafficHazard >= 0.35 {
            return "watch the road \(formatScalar(trafficHazard))"
        }
        return "traffic calm \(formatScalar(trafficHazard))"
    }

    private static func combatHUDModel(state: MDTBEngineState, interactionSummary: String, trafficHazard: Float, blocks: [SceneBlock], roadLinks: [SceneRoadLink]) -> CombatHUDModel {
        let healthRatio = Double(max(0.0, min(state.player_health / 100.0, 1.0)))
        let anchorLabel = lookoutAnchorLabel(state.combat_hostile_anchor_index)
        let focusLabel = combatTargetLabel(state.combat_focus_target_kind)
        let isVehicleMode = state.traversal_mode == UInt32(MDTBTraversalModeVehicle)
        let isResetting = state.player_reset_timer > 0.0
        let streetIncidentActive = state.street_incident_timer > 0.0 && state.street_incident_level > 0.08
        let searchActive = state.combat_hostile_search_timer > 0.0
        let recoveryActive = streetRecoveryIntensity(state: state) > 0.08
        let territoryClaimed =
            state.territory_faction != UInt32(MDTBTerritoryFactionNone) &&
            state.territory_phase != UInt32(MDTBTerritoryPhaseNone)
        let territoryHot = state.territory_phase == UInt32(MDTBTerritoryPhaseHot)
        let territoryVehicleEntry = state.territory_entry_mode == UInt32(MDTBTerritoryEntryVehicle)
        let territorySkimming = territorySkimmingLine(state: state)
        let territoryDeeper = territoryPushingDeeper(state: state)
        let territoryWatch = territoryWatchIntensity(state: state)
        let territoryPatrolWatch = territoryPatrolWatchActive(state: state)
        let territoryPatrolScreen = territoryPatrolScreening(state: state)
        let territoryPatrolClear = territoryPatrolClearing(state: state)
        let territoryPatrolReform = territoryPatrolReforming(state: state)
        let territoryPatrolBrace = territoryPatrolHardening(state: state)
        let territoryPatrolHandoff = state.territory_patrol_state == UInt32(MDTBTerritoryPatrolHandoff)
        let territoryInnerWatch = territoryInnerWatchActive(state: state)
        let territoryEntryClamp = territoryEntryClamped(state: state)
        let territoryCommitWindowOpen = territoryCommitWindowActive(state: state)
        let territoryCommitLive = territoryCommitActive(state: state)
        let territoryCommitResolved = territoryCommitComplete(state: state)
        let territoryCommitProgress = min(max(state.territory_commit_progress, 0.0), 1.0)
        let territoryResolveWindowOpen = territoryResolveWindowActive(state: state)
        let territoryResolveHoldLive = territoryResolveHoldActive(state: state)
        let territoryResolvePulloutLive = territoryResolvePulloutActive(state: state)
        let territoryResolveProgress = min(max(state.territory_resolve_progress, 0.0), 1.0)
        let territoryResolveHeld = territoryResolveHoldLive && territoryResolveProgress > 0.99
        let territoryResolveClean = territoryResolvePulloutLive && territoryResolveProgress > 0.99
        let territoryReclaimReturn = territoryReclaimReturnActive(state: state)
        let territoryRetakeReturn = territoryRetakeReturnActive(state: state)
        let territoryReclaimFeint = territoryReclaimFeint(state: state)
        let territoryReclaimCommit = territoryReclaimCommit(state: state)
        let territoryReclaimCounter = territoryReclaimCounterStep(state: state)
        let territoryRetakeChallenge = territoryRetakeChallenge(state: state)
        let territoryRetakeCommit = territoryRetakeCommit(state: state)
        let territoryRetakeCross = territoryRetakeCrossAngle(state: state)
        let territoryPreferredShoulder = territoryPreferredShoulderLabel(state: state)
        let lookoutShoulder = lookoutAnchorShoulderLabel(state.combat_hostile_anchor_index)
        let territoryReapproachTimer = formatScalar(state.territory_reapproach_timer)
        let lookoutTitle = territoryClaimed
            ? "\(territoryFactionLabel(state.territory_faction)) lookout \(anchorLabel)"
            : "Lookout \(anchorLabel)"
        let isCritical = isResetting || state.player_health <= 35.0 || (state.combat_hostile_attack_windup > 0.0 && state.combat_player_in_cover == 0)
        let systemStatus = systemicHUDStatus(state: state, trafficHazard: trafficHazard)
        let mapStatus = mapStatus(state: state, blocks: blocks, roadLinks: roadLinks)

        let title: String
        let subtitle: String
        let healthDetail: String
        let weaponTitle: String
        let weaponDetail: String
        let encounterTitle: String
        let encounterDetail: String
        let systemTitle = systemStatus.title
        let systemDetail = systemStatus.detail

        if isResetting {
            title = "Lane Reset"
            subtitle = territoryHot ? "Regroup before stepping back into a burned block" : "Regroup before stepping back into the pocket"
            healthDetail = "Encounter recenter \(formatScalar(state.player_reset_timer))s"
            encounterTitle = "Lookout reset"
            encounterDetail = "Green window in \(formatScalar(state.player_reset_timer))s"
        } else if territoryResolveHeld || territoryResolveClean || territoryResolveHoldLive || territoryResolvePulloutLive || territoryResolveWindowOpen {
            if territoryResolveHeld {
                title = "Pocket Reclaim"
                subtitle = territoryPreferredShoulder != nil
                    ? (isVehicleMode
                        ? "You held the break and the block is reclaiming deeper around the \(territoryPreferredShoulder!) shoulder of the vehicle pocket"
                        : "You held the break and the block is reclaiming deeper around the \(territoryPreferredShoulder!) shoulder of the pocket")
                    : (isVehicleMode
                        ? "You held the break and the block is reclaiming deeper around the vehicle pocket"
                        : "You held the break and the block is reclaiming deeper around the pocket")
                healthDetail = state.player_health < 100.0 && state.player_recovery_delay > 0.0
                    ? (territoryPreferredShoulder != nil
                        ? "Recovery in \(formatScalar(state.player_recovery_delay))s / \(territoryPreferredShoulder!) shoulder reclaim live"
                        : "Recovery in \(formatScalar(state.player_recovery_delay))s / reclaim live")
                    : (territoryPreferredShoulder != nil
                        ? "\(territoryPreferredShoulder!) shoulder reclaim live / pressure still hot"
                        : "Reclaim live / pressure still hot")
                encounterTitle = lookoutShoulder == territoryPreferredShoulder && territoryPreferredShoulder != nil
                    ? lookoutTitle
                    : "Pocket reclaim"
                encounterDetail = lookoutShoulder == territoryPreferredShoulder && territoryPreferredShoulder != nil
                    ? "\(anchorLabel) is leaning into the \(territoryPreferredShoulder!) shoulder reclaim \(formatScalar(state.territory_resolve_timer))s"
                    : (territoryPreferredShoulder != nil
                        ? "Reclaim \(formatScalar(state.territory_resolve_timer))s / \(territoryPreferredShoulder!) shoulder carry"
                        : "Reclaim \(formatScalar(state.territory_resolve_timer))s / deep \(formatScalar(state.territory_deep_watch))")
            } else if territoryResolveClean {
                title = "Edge Retaking"
                subtitle = searchActive
                    ? (territoryPreferredShoulder != nil
                        ? "You got out clean, but the \(territoryPreferredShoulder!) shoulder is sealing behind your exit"
                        : "You got out clean, but the edge is sealing behind your exit")
                    : (territoryPreferredShoulder != nil
                        ? "Runner and post are resealing the \(territoryPreferredShoulder!) shoulder of the lane mouth behind you"
                        : "Runner and post are resealing the lane mouth behind you")
                healthDetail = searchActive
                    ? (territoryPreferredShoulder != nil
                        ? "Exit search \(formatScalar(state.combat_hostile_search_timer))s / \(territoryPreferredShoulder!) shoulder resealing"
                        : "Exit search \(formatScalar(state.combat_hostile_search_timer))s / edge resealing")
                    : (territoryPreferredShoulder != nil
                        ? "Street settling while the \(territoryPreferredShoulder!) shoulder retakes"
                        : "Street settling while the edge retakes")
                encounterTitle = lookoutShoulder == territoryPreferredShoulder && territoryPreferredShoulder != nil
                    ? lookoutTitle
                    : "Edge retake"
                encounterDetail = lookoutShoulder == territoryPreferredShoulder && territoryPreferredShoulder != nil
                    ? (searchActive
                        ? "\(anchorLabel) is checking the \(territoryPreferredShoulder!) shoulder while it reseals \(formatScalar(state.combat_hostile_search_timer))s"
                        : "\(anchorLabel) is sealing the \(territoryPreferredShoulder!) shoulder \(formatScalar(state.territory_resolve_timer))s")
                    : (territoryPreferredShoulder != nil
                        ? "Retake \(formatScalar(state.territory_resolve_timer))s / \(territoryPreferredShoulder!) shoulder sealing"
                        : "Retake \(formatScalar(state.territory_resolve_timer))s / front \(formatScalar(state.territory_front_watch))")
            } else if territoryResolveHoldLive {
                title = "Hold The Pocket"
                subtitle = territoryPreferredShoulder != nil
                    ? "Stay inside a beat longer and force the block to reclaim deeper turf on the \(territoryPreferredShoulder!) shoulder"
                    : "Stay inside a beat longer and force the block to reclaim deeper turf"
                healthDetail = state.player_health < 100.0 && state.player_recovery_delay > 0.0
                    ? "Recovery in \(formatScalar(state.player_recovery_delay))s"
                    : (territoryPreferredShoulder != nil
                        ? "Hold inside until the \(territoryPreferredShoulder!) shoulder carry lands"
                        : "Hold inside until the follow-through lands")
                encounterTitle = "Pocket reclaim"
                encounterDetail = territoryPreferredShoulder != nil
                    ? "Hold \(formatScalar(territoryResolveProgress)) / \(territoryPreferredShoulder!) shoulder"
                    : "Hold \(formatScalar(territoryResolveProgress)) / \(formatScalar(state.territory_resolve_timer))s"
            } else if territoryResolvePulloutLive {
                title = "Pull Out Clean"
                subtitle = territoryPreferredShoulder != nil
                    ? "Back out through the mouth before the \(territoryPreferredShoulder!) shoulder reseal locks in"
                    : "Back out through the mouth before the edge retake locks in"
                healthDetail = searchActive
                    ? "Exit search already live"
                    : (territoryPreferredShoulder != nil
                        ? "Leave cleanly while the \(territoryPreferredShoulder!) shoulder is still yours"
                        : "Leave cleanly while the break is still yours")
                encounterTitle = "Edge retake"
                encounterDetail = territoryPreferredShoulder != nil
                    ? "Pullout \(formatScalar(territoryResolveProgress)) / \(territoryPreferredShoulder!) shoulder"
                    : "Pullout \(formatScalar(territoryResolveProgress)) / \(formatScalar(state.territory_resolve_timer))s"
            } else {
                title = "Resolve Beat"
                subtitle = "Hold the pocket or peel out before the block chooses reclaim or retake"
                healthDetail = "Choose the follow-through"
                encounterTitle = "Line follow-through"
                encounterDetail = "Choose \(formatScalar(state.territory_resolve_timer))s / break is live"
            }
        } else if territoryReclaimReturn || territoryRetakeReturn {
            if territoryReclaimReturn {
                if territoryReclaimFeint {
                    title = "Pocket Feint"
                    subtitle = territoryReclaimCounter
                        ? (territoryPreferredShoulder != nil
                            ? "Runner is counter-stepping off the \(territoryPreferredShoulder!) shoulder while the inner post turns the next angle deeper"
                            : "Runner is counter-stepping off the mouth while the inner post turns the next angle deeper")
                        : "The edge is showing soft, but the inner post is already waiting deeper for the next commit"
                    healthDetail = territoryReclaimCounter
                        ? (territoryPreferredShoulder != nil
                            ? "Reclaim return \(territoryReapproachTimer)s / \(territoryPreferredShoulder!) shoulder favored"
                            : "Reclaim return \(territoryReapproachTimer)s / counter-step live")
                        : "Reclaim return \(territoryReapproachTimer)s / soft edge, deep pickup"
                    encounterTitle = "Reclaim feint"
                    encounterDetail = territoryReclaimCounter
                        ? (territoryPreferredShoulder != nil
                            ? "Counter-step \(territoryReapproachTimer)s / \(territoryPreferredShoulder!) shoulder"
                            : "Counter-step \(territoryReapproachTimer)s / deep \(formatScalar(state.territory_deep_watch))")
                        : "Feint \(territoryReapproachTimer)s / deep \(formatScalar(state.territory_deep_watch))"
                } else if territoryReclaimCommit {
                    title = "Reclaim Commit"
                    subtitle = territoryPreferredShoulder != nil
                        ? "You took the softer \(territoryPreferredShoulder!) shoulder and the block is snapping the next handoff deeper around that pocket"
                        : "You took the softer mouth and the block is snapping the next handoff deeper around the pocket"
                    healthDetail = territoryPreferredShoulder != nil
                        ? "Reclaim return \(territoryReapproachTimer)s / \(territoryPreferredShoulder!) shoulder carrying deeper"
                        : "Reclaim return \(territoryReapproachTimer)s / pocket committing"
                    encounterTitle = "Reclaim commit"
                    encounterDetail = territoryPreferredShoulder != nil
                        ? "Commit \(territoryReapproachTimer)s / \(territoryPreferredShoulder!) shoulder carry"
                        : "Commit \(territoryReapproachTimer)s / deep \(formatScalar(state.territory_deep_watch))"
                } else {
                    title = territoryClaimed ? "Reclaim Return" : "Pocket Remembered"
                    subtitle = territoryClaimed
                        ? (territoryPreferredShoulder != nil
                            ? "The lane mouth is looser on the \(territoryPreferredShoulder!) shoulder, but the inner post and lookout are reading the next pass deeper"
                            : "The lane mouth is looser, but the inner post and lookout are reading the next pass deeper")
                        : "The last hold still hangs over the block, so the pocket is waking faster than the edge"
                    healthDetail = territoryClaimed
                        ? (territoryPreferredShoulder != nil
                            ? "Reclaim return \(territoryReapproachTimer)s / \(territoryPreferredShoulder!) shoulder remembered"
                            : "Reclaim return \(territoryReapproachTimer)s / deep watch carries")
                        : "Pocket memory \(territoryReapproachTimer)s / mouth lighter"
                    encounterTitle = "Pocket memory"
                    encounterDetail = "Return \(territoryReapproachTimer)s / deep \(formatScalar(state.territory_deep_watch))"
                }
            } else {
                if territoryRetakeChallenge {
                    title = "Edge Challenge"
                    subtitle = territoryRetakeCross
                        ? (territoryPreferredShoulder != nil
                            ? "Runner and post are cross-angling the \(territoryPreferredShoulder!) shoulder of the lane mouth before the heavier recommit lands"
                            : "Runner and post are cross-angling the lane mouth before the heavier recommit lands")
                        : (searchActive
                            ? "The last pullout is still on the lane mouth and the lookout is checking whether you really want back in"
                            : "The runner and post are throwing an earlier challenge at the mouth before they commit deeper")
                    healthDetail = territoryRetakeCross
                        ? (territoryPreferredShoulder != nil
                            ? "Retake return \(territoryReapproachTimer)s / \(territoryPreferredShoulder!) shoulder favored"
                            : "Retake return \(territoryReapproachTimer)s / cross-angle live")
                        : (searchActive
                            ? "Search \(formatScalar(state.combat_hostile_search_timer))s / challenge still live"
                            : "Retake return \(territoryReapproachTimer)s / edge challenging early")
                    encounterTitle = "Edge challenge"
                    encounterDetail = territoryRetakeCross
                        ? (territoryPreferredShoulder != nil
                            ? "Cross-angle \(territoryReapproachTimer)s / \(territoryPreferredShoulder!) shoulder"
                            : "Cross-angle \(territoryReapproachTimer)s / front \(formatScalar(state.territory_front_watch))")
                        : "Challenge \(territoryReapproachTimer)s / front \(formatScalar(state.territory_front_watch))"
                } else if territoryRetakeCommit {
                    title = "Retake Commit"
                    subtitle = territoryPreferredShoulder != nil
                        ? "You hit the early \(territoryPreferredShoulder!) shoulder challenge and the block is recommitting through that side instead of giving the same cold lane"
                        : "You hit the early challenge and the block is recommitting through the mouth instead of giving the same cold lane"
                    healthDetail = territoryPreferredShoulder != nil
                        ? "Retake return \(territoryReapproachTimer)s / \(territoryPreferredShoulder!) shoulder resealing"
                        : "Retake return \(territoryReapproachTimer)s / recommit live"
                    encounterTitle = "Retake commit"
                    encounterDetail = territoryPreferredShoulder != nil
                        ? "Recommit \(territoryReapproachTimer)s / \(territoryPreferredShoulder!) shoulder seal"
                        : "Recommit \(territoryReapproachTimer)s / front \(formatScalar(state.territory_front_watch))"
                } else {
                    title = territoryClaimed ? "Retake Return" : "Edge Remembered"
                    subtitle = searchActive
                        ? "The clean exit is still sitting on the lane mouth while the lookout checks the return"
                        : (territoryPreferredShoulder != nil
                            ? "Runner and post are meeting this pass earlier on the \(territoryPreferredShoulder!) shoulder than the last one"
                            : "Runner and post are meeting this pass at the edge sooner than the last one")
                    healthDetail = searchActive
                        ? "Search \(formatScalar(state.combat_hostile_search_timer))s / edge still remembered"
                        : (territoryPreferredShoulder != nil
                            ? "Retake return \(territoryReapproachTimer)s / \(territoryPreferredShoulder!) shoulder remembered"
                            : "Retake return \(territoryReapproachTimer)s / mouth closing early")
                    encounterTitle = "Edge memory"
                    encounterDetail = "Return \(territoryReapproachTimer)s / front \(formatScalar(state.territory_front_watch))"
                }
            }
        } else if isVehicleMode {
            title = searchActive
                ? "Vehicle Escape"
                : (territoryCommitResolved
                    ? "Line Broken"
                    : (territoryCommitLive
                        ? "Vehicle Commit"
                        : (!territoryClaimed && territoryPatrolWatch
                            ? (territoryCommitWindowOpen ? "Commit Window" : (territoryPatrolBrace ? "Vehicle Hardening" : (territoryPatrolReform ? "Vehicle Reforming" : (territoryPatrolScreen ? "Vehicle Screened" : "Vehicle Watched"))))
                            : (territoryClaimed && territoryWatch > 0.12
                                ? (territoryDeeper ? "Vehicle Push" : (territoryPatrolBrace ? "Vehicle Hardening" : (territoryPatrolReform ? "Vehicle Reforming" : "Vehicle Line")))
                                : "Vehicle Reset"))))
            if searchActive {
                subtitle = streetIncidentActive
                    ? "The lane spilled into the street while the lookout checks your exit"
                    : "The lookout is searching your exit point while you stay mobile"
            } else if territoryCommitResolved {
                subtitle = "You drove through the hardened edge and broke the line"
            } else if territoryCommitLive {
                subtitle = "Stay through the break while the line gives way"
            } else if territoryHot {
                subtitle = "The block remembers the last pass while you stay mobile"
            } else if !territoryClaimed && territoryPatrolWatch {
                subtitle = territoryCommitWindowOpen
                    ? "Push through the hardened line before it settles"
                    : (territoryPatrolHandoff
                        ? "The sidewalk runner is turning your vehicle toward the lane"
                        : (territoryPatrolBrace
                            ? "The runner is locking the vehicle line back down"
                            : (territoryPatrolReform
                            ? (recoveryActive && territoryPreferredShoulder != nil
                                ? "The runner is reforming the \(territoryPreferredShoulder!) shoulder while spillback reopens the vehicle line"
                                : "The runner is reforming the vehicle line after the clear")
                            : (territoryPatrolScreen
                            ? "The sidewalk runner is screening the vehicle at the lane mouth"
                            : "A court-set runner is reading the vehicle before the line"))))
            } else if territoryClaimed && territoryWatch > 0.12 {
                if territoryCommitLive {
                    subtitle = "Stay through the break while the line gives way"
                } else if territoryCommitResolved {
                    subtitle = "You drove through the hardened edge and broke the line"
                } else if territorySkimming {
                    subtitle = territoryPatrolBrace
                        ? "Runner and post are hardening the vehicle line back into clamp"
                        : territoryEntryClamp
                        ? "Runner and post are clamping the vehicle line from opposite angles"
                        : (territoryPatrolReform
                            ? "The runner is reforming the vehicle line while the post eases back in"
                            : (territoryPatrolScreen
                            ? "The runner is screening the vehicle line while the post stays deeper"
                            : "Rolling the court-set line while the front hold clocks you"))
                } else {
                    subtitle = territoryPatrolClear && territoryInnerWatch
                        ? "Runner cleared the line while the inner post keeps the vehicle boxed in"
                        : "Vehicle push is drawing the inner post inside claimed turf"
                }
            } else if recoveryActive {
                subtitle = "The block is reopening while you stay mobile"
            } else if state.combat_hostile_reacquire_timer > 0.0 {
                subtitle = "Lookout is cooling off while you stay mobile"
            } else {
                subtitle = "Weapons are stowed while you drive"
            }
            healthDetail = state.player_health < 100.0 ? "Pulled back / \(formatScalar(state.player_health))hp" : "Pulled back from the lane"
            encounterTitle = territoryCommitWindowOpen || territoryCommitLive || territoryCommitResolved
                ? "Line objective"
                : (!territoryClaimed && territoryPatrolWatch
                    ? (territoryPatrolBrace ? "Court-set hardening" : (territoryPatrolReform ? "Court-set reform" : (territoryPatrolScreen ? "Court-set screen" : "Court-set patrol")))
                    : (territoryClaimed && territoryWatch > 0.12
                        ? (territoryDeeper && territoryInnerWatch
                            ? (territoryPatrolClear ? "Court-set clear" : "Court-set bodies")
                            : (territoryPatrolBrace
                                ? "Court-set hardening"
                                : (territoryEntryClamp
                                ? "Court-set clamp"
                                : (territorySkimming && territoryPatrolReform
                                    ? "Court-set reform"
                                    : (territorySkimming && territoryPatrolScreen
                                    ? "Court-set screen"
                                    : "\(territoryFactionLabel(state.territory_faction)) holds"))))
                        )
                        : (territoryClaimed ? "\(territoryFactionLabel(state.territory_faction)) hold" : "Lookout holding \(anchorLabel)")))
            if searchActive {
                let reacquireTail = state.combat_hostile_reacquire_timer > 0.0
                    ? " / reacquire \(formatScalar(state.combat_hostile_reacquire_timer))s after you step out"
                    : ""
                encounterDetail = "Search \(formatScalar(state.combat_hostile_search_timer))s\(reacquireTail)"
            } else if territoryCommitResolved {
                encounterDetail = "Complete / settle \(formatScalar(state.territory_commit_timer))s"
            } else if territoryCommitLive {
                encounterDetail = "Commit \(formatScalar(territoryCommitProgress)) / \(formatScalar(state.territory_commit_timer))s"
            } else if !territoryClaimed && territoryPatrolWatch {
                if territoryCommitWindowOpen {
                    encounterDetail = "Window \(formatScalar(state.territory_commit_timer))s / brace \(formatScalar(state.territory_patrol_alert))"
                } else if territoryPatrolHandoff {
                    encounterDetail = "Handoff \(formatScalar(state.territory_patrol_alert)) / line closing"
                } else if territoryPatrolBrace {
                    encounterDetail = "Brace \(formatScalar(state.territory_patrol_alert)) / line locking"
                } else if territoryPatrolReform {
                    encounterDetail = recoveryActive && territoryPreferredShoulder != nil
                        ? "Reform \(formatScalar(state.territory_patrol_alert)) / \(territoryPreferredShoulder!) shoulder spillback"
                        : "Reform \(formatScalar(state.territory_patrol_alert)) / line retaking"
                } else if territoryPatrolScreen {
                    encounterDetail = "Screen \(formatScalar(state.territory_patrol_alert)) / entry pinched"
                } else {
                    encounterDetail = "Sidewalk watch \(formatScalar(state.territory_patrol_alert))"
                }
            } else if territoryClaimed && territoryWatch > 0.12 {
                if territoryCommitResolved {
                    encounterDetail = "Complete / settle \(formatScalar(state.territory_commit_timer))s"
                } else if territoryCommitLive {
                    encounterDetail = "Commit \(formatScalar(territoryCommitProgress)) / \(formatScalar(state.territory_commit_timer))s"
                } else if territoryDeeper && territoryInnerWatch {
                    if territoryPatrolClear {
                        encounterDetail = "Clear \(formatScalar(state.territory_patrol_alert)) / post \(formatScalar(state.territory_inner_alert))"
                    } else if territoryPatrolReform {
                        encounterDetail = "Reform \(formatScalar(state.territory_patrol_alert)) / post \(formatScalar(state.territory_inner_alert))"
                    } else {
                        encounterDetail = "Runner \(formatScalar(state.territory_patrol_alert)) / post \(formatScalar(state.territory_inner_alert))"
                    }
                } else if territoryPatrolBrace {
                    encounterDetail = "Brace \(formatScalar(state.territory_patrol_alert)) / post \(formatScalar(state.territory_inner_alert))"
                } else if territoryEntryClamp {
                    encounterDetail = "Clamp \(formatScalar(state.territory_patrol_alert)) / post \(formatScalar(state.territory_inner_alert))"
                } else if territorySkimming && territoryPatrolReform {
                    encounterDetail = "Reform \(formatScalar(state.territory_patrol_alert)) / post \(formatScalar(state.territory_inner_alert))"
                } else if territorySkimming && territoryPatrolScreen {
                    encounterDetail = "Screen \(formatScalar(state.territory_patrol_alert)) / line pinched"
                } else if territoryDeeper {
                    encounterDetail = "Front \(formatScalar(state.territory_front_watch)) / deep \(formatScalar(state.territory_deep_watch))"
                } else {
                    encounterDetail = "Front hold \(formatScalar(state.territory_front_watch)) / watch \(formatScalar(state.territory_watch_timer))s"
                }
            } else {
                encounterDetail = state.combat_hostile_reacquire_timer > 0.0
                    ? "Reacquire \(formatScalar(state.combat_hostile_reacquire_timer))s after you step out"
                    : "Exit near cover to re-engage"
            }
        } else if state.combat_hostile_attack_windup > 0.0 {
            title = "Incoming Fire"
            subtitle = "Break the angle from \(anchorLabel) or fire first"
            healthDetail = state.combat_player_in_cover != 0 ? "Cover is still holding" : "Open lane / move now"
            encounterTitle = lookoutTitle
            encounterDetail = "Firing in \(formatScalar(state.combat_hostile_attack_windup))s"
        } else if searchActive {
            title = state.combat_player_in_cover != 0 ? "Search Window" : "Hostile Searching"
            subtitle = territoryHot
                ? "You came back hot and the block is reading your re-entry"
                : (state.witness_state == UInt32(MDTBWitnessStateFlee) || streetIncidentActive
                ? "You broke the angle, but the street is still reacting around the lane"
                : "The lookout is checking your last seen position instead of firing blind")
            healthDetail = state.player_health < 100.0 && state.player_recovery_delay > 0.0
                ? "Recovery window \(formatScalar(state.player_recovery_delay))s"
                : (state.combat_player_in_cover != 0 ? "Cover bought a short window" : "Stay mobile and re-enter on your terms")
            encounterTitle = lookoutTitle
            encounterDetail = "Search \(formatScalar(state.combat_hostile_search_timer))s around last seen position"
        } else if !territoryClaimed && territoryPatrolWatch && !streetIncidentActive {
            title = territoryCommitWindowOpen ? "Commit Window" : (territoryPatrolBrace ? "Line Hardening" : (territoryPatrolReform ? "Line Reforming" : (territoryPatrolScreen ? "Entry Challenged" : "Sidewalk Watched")))
            subtitle = territoryCommitWindowOpen
                ? "Push through the hardened line before it settles"
                : (territoryPatrolHandoff
                    ? "The sidewalk runner is turning the lane toward the lookout"
                    : (territoryPatrolBrace
                        ? "The runner is locking the line back down"
                        : (territoryPatrolReform
                        ? (recoveryActive && territoryPreferredShoulder != nil
                            ? "The runner is reforming the \(territoryPreferredShoulder!) shoulder while spillback reopens the lane"
                            : "The runner is reforming the line after the clear")
                        : (territoryPatrolScreen
                        ? "The sidewalk runner is stepping across the lane mouth"
                        : "A court-set runner is posted outside the lane mouth"))))
            healthDetail = state.player_health < 100.0 && state.player_recovery_delay > 0.0
                ? "Recovery in \(formatScalar(state.player_recovery_delay))s"
                : "Pressure is building before you cross the line"
            encounterTitle = territoryCommitWindowOpen ? "Line objective" : (territoryPatrolBrace ? "Court-set hardening" : (territoryPatrolReform ? "Court-set reform" : (territoryPatrolScreen ? "Court-set screen" : "Court-set patrol")))
            if territoryCommitWindowOpen {
                encounterDetail = "Window \(formatScalar(state.territory_commit_timer))s / brace \(formatScalar(state.territory_patrol_alert))"
            } else if territoryPatrolHandoff {
                encounterDetail = "Handoff \(formatScalar(state.territory_patrol_alert)) / lane mouth closing"
            } else if territoryPatrolBrace {
                encounterDetail = "Brace \(formatScalar(state.territory_patrol_alert)) / line locking"
            } else if territoryPatrolReform {
                encounterDetail = recoveryActive && territoryPreferredShoulder != nil
                    ? "Reform \(formatScalar(state.territory_patrol_alert)) / \(territoryPreferredShoulder!) shoulder spillback"
                    : "Reform \(formatScalar(state.territory_patrol_alert)) / line retaking"
            } else if territoryPatrolScreen {
                encounterDetail = "Screen \(formatScalar(state.territory_patrol_alert)) / lane mouth pinched"
            } else {
                encounterDetail = "Sidewalk watch \(formatScalar(state.territory_patrol_alert))"
            }
        } else if territoryClaimed && territoryWatch > 0.12 && !streetIncidentActive {
            title = territoryCommitResolved
                ? "Line Broken"
                : (territoryCommitLive
                    ? "Hold The Pocket"
                    : (territorySkimming
                        ? (territoryPatrolBrace ? "Territory Hardening" : (territoryEntryClamp ? "Territory Clamp" : (territoryPatrolReform ? "Territory Reforming" : (territoryPatrolScreen ? "Territory Screen" : "Territory Line"))))
                        : "Claimed Block"))
            subtitle = territoryCommitResolved
                ? "You pushed through the hardened edge"
                : (territoryCommitLive
                    ? "Stay through the break while the line gives way"
                    : (territorySkimming
                        ? (territoryVehicleEntry
                            ? (territoryPatrolBrace
                                ? "Runner and post are hardening the vehicle line back into clamp"
                                : (territoryEntryClamp
                                ? "Runner and post are clamping the vehicle line before the lane clears"
                                : (territoryPatrolReform
                                    ? "The runner is reforming the vehicle line while the post eases back in"
                                    : (territoryPatrolScreen
                                    ? "The runner is screening the vehicle at the line while the post stays deeper"
                                    : "The front hold is still reading your vehicle at the lane mouth"))))
                            : (territoryPatrolBrace
                                ? "Runner and post are hardening the edge back into clamp"
                                : (territoryEntryClamp
                                ? "Runner and post are clamping the entry from opposite angles"
                                : (territoryPatrolReform
                                    ? "The runner is reforming the edge while the post eases back toward support"
                                    : (territoryPatrolScreen
                                    ? "The runner is screening the entry before the handoff opens"
                                    : "The front hold is reading you on foot at the lane mouth")))))
                        : (territoryVehicleEntry
                            ? (territoryPatrolClear && territoryInnerWatch
                                ? "Runner cleared the line while the inner post owns the vehicle handoff"
                                : "Vehicle push is drawing the inner post inside claimed turf")
                            : (territoryPatrolClear && territoryInnerWatch
                                ? "Runner cleared the line while the inner post owns the pocket"
                                : "Deeper push into claimed turf is waking the inner post"))))
            healthDetail = state.player_health < 100.0 && state.player_recovery_delay > 0.0
                ? "Recovery in \(formatScalar(state.player_recovery_delay))s"
                : "Pressure is building before the lane breaks open"
            encounterTitle = territoryCommitLive || territoryCommitResolved
                ? "Line objective"
                : (territoryDeeper && territoryInnerWatch
                    ? (territoryPatrolClear ? "Court-set clear" : "Court-set bodies")
                    : (territoryPatrolBrace
                        ? "Court-set hardening"
                        : (territoryEntryClamp
                        ? "Court-set clamp"
                        : (territorySkimming && territoryPatrolReform
                            ? "Court-set reform"
                            : (territorySkimming && territoryPatrolScreen
                            ? "Court-set screen"
                            : "\(territoryFactionLabel(state.territory_faction)) holds")))))
            if territoryCommitResolved {
                encounterDetail = "Complete / settle \(formatScalar(state.territory_commit_timer))s"
            } else if territoryCommitLive {
                encounterDetail = "Commit \(formatScalar(territoryCommitProgress)) / \(formatScalar(state.territory_commit_timer))s"
            } else if territoryDeeper && territoryInnerWatch {
                if territoryPatrolClear {
                    encounterDetail = "Clear \(formatScalar(state.territory_patrol_alert)) / post \(formatScalar(state.territory_inner_alert))"
                } else if territoryPatrolReform {
                    encounterDetail = "Reform \(formatScalar(state.territory_patrol_alert)) / post \(formatScalar(state.territory_inner_alert))"
                } else {
                    encounterDetail = "Runner \(formatScalar(state.territory_patrol_alert)) / post \(formatScalar(state.territory_inner_alert))"
                }
            } else if territoryPatrolBrace {
                encounterDetail = "Brace \(formatScalar(state.territory_patrol_alert)) / post \(formatScalar(state.territory_inner_alert))"
            } else if territoryEntryClamp {
                encounterDetail = "Clamp \(formatScalar(state.territory_patrol_alert)) / post \(formatScalar(state.territory_inner_alert))"
            } else if territorySkimming && territoryPatrolReform {
                encounterDetail = "Reform \(formatScalar(state.territory_patrol_alert)) / post \(formatScalar(state.territory_inner_alert))"
            } else if territorySkimming && territoryPatrolScreen {
                encounterDetail = "Screen \(formatScalar(state.territory_patrol_alert)) / line pinched"
            } else if territoryDeeper {
                encounterDetail = "Front \(formatScalar(state.territory_front_watch)) / deep \(formatScalar(state.territory_deep_watch))"
            } else {
                encounterDetail = "Front hold \(formatScalar(state.territory_front_watch)) / watch \(formatScalar(state.territory_watch_timer))s"
            }
        } else if state.combat_player_in_cover != 0 {
            title = "Cover Holding"
            subtitle = state.combat_hostile_reacquire_timer > 0.0
                ? "The lookout is shifting off \(anchorLabel)"
                : "Use the pocket to reload or swing the lane"
            healthDetail = state.player_health < 100.0 && state.player_recovery_delay > 0.0
                ? "Recovery in \(formatScalar(state.player_recovery_delay))s"
                : "Cover holding"
            encounterTitle = lookoutTitle
            encounterDetail = state.combat_hostile_reacquire_timer > 0.0
                ? "Reacquire \(formatScalar(state.combat_hostile_reacquire_timer))s"
                : "Pressure broken on this angle"
        } else if recoveryActive {
            title = "Street Reopening"
            subtitle = territoryHot
                ? "The territory is cooling, but the block still remembers you"
                : (state.combat_hostile_reacquire_timer > 0.0
                ? "The lookout is reset, but the block is still settling"
                : (territoryPreferredShoulder != nil
                    ? "The \(territoryPreferredShoulder!) shoulder is reopening while the block settles"
                    : "Traffic and civilians are finding their default rhythm again"))
            healthDetail = state.player_health < 100.0
                ? (state.player_recovery_delay > 0.0
                    ? "Recovery \(formatScalar(state.player_recovery_delay))s / block settling"
                    : "Recovering while the block settles")
                : "Lane quiet / block settling"
            encounterTitle = lookoutTitle
            encounterDetail = state.combat_hostile_reacquire_timer > 0.0
                ? "Reacquire tail \(formatScalar(state.combat_hostile_reacquire_timer))s"
                : "Threat broken / watch the street reset"
        } else {
            title = state.player_health <= 45.0 ? "Under Pressure" : "Combat Lane Live"
            subtitle = territoryHot
                ? "You came back into a burned block / move before it locks in"
                : (state.combat_hostile_reacquire_timer > 0.0
                ? "The lookout is sliding to a new firing angle"
                : "Move between cover so the lane does not flatten out")
            if state.player_health < 100.0 && state.player_recovery_delay > 0.0 {
                healthDetail = "Recovery in \(formatScalar(state.player_recovery_delay))s"
            } else if state.player_health < 100.0 {
                healthDetail = "Recovering"
            } else {
                healthDetail = "Open lane"
            }
            encounterTitle = lookoutTitle
            if state.combat_hostile_last_shot_timer > 0.0 {
                encounterDetail = state.combat_hostile_last_shot_hit != 0 ? "Last burst landed" : "Last burst splashed cover"
            } else if state.combat_hostile_reacquire_timer > 0.0 {
                encounterDetail = "Reacquire \(formatScalar(state.combat_hostile_reacquire_timer))s"
            } else if state.combat_focus_target_kind != UInt32(MDTBCombatTargetNone) {
                encounterDetail = "Focus \(focusLabel) \(formatScalar(state.combat_focus_distance))m"
            } else {
                encounterDetail = "Search the lane and take the better angle"
            }
        }

        switch state.equipped_weapon_kind {
        case UInt32(MDTBEquippedWeaponLeadPipe):
            weaponTitle = "Lead Pipe"
            weaponDetail = state.melee_attack_phase == UInt32(MDTBMeleeAttackIdle)
                ? "Close distance / slot 1 ready"
                : "\(attackPhaseLabel(state.melee_attack_phase)) \(formatScalar(state.melee_attack_timer))s"
        case UInt32(MDTBEquippedWeaponPistol):
            weaponTitle = isVehicleMode ? "Pistol Stowed" : "Pistol"
            if state.firearm_reloading != 0 {
                weaponDetail = "Reload \(formatScalar(state.firearm_reload_timer))s / \(pistolAmmoSummary(state: state))"
            } else {
                weaponDetail = pistolAmmoSummary(state: state)
            }
        default:
            weaponTitle = "Unarmed"
            if state.melee_weapon_owned != 0 || state.firearm_owned != 0 {
                weaponDetail = "Press 1 or 2 to equip"
            } else {
                weaponDetail = "Press T near the lane pickups"
            }
        }

        return CombatHUDModel(
            title: title,
            subtitle: subtitle,
            healthRatio: healthRatio,
            healthText: "\(formatScalar(state.player_health)) hp",
            healthDetail: healthDetail,
            weaponTitle: weaponTitle,
            weaponDetail: weaponDetail,
            encounterTitle: encounterTitle,
            encounterDetail: encounterDetail,
            systemTitle: systemTitle,
            systemDetail: systemDetail,
            mapTitle: mapStatus.title,
            mapDetail: mapStatus.detail,
            promptText: interactionSummary,
            isCritical: isCritical,
            isVehicleMode: isVehicleMode,
            isResetting: isResetting
        )
    }

    private static func lookoutAnchorLabel(_ value: UInt32) -> String {
        switch value {
        case 0:
            return "west stack"
        case 1:
            return "center slit"
        case 2:
            return "east swing"
        case 3:
            return "backline"
        default:
            return "lane"
        }
    }

    private static func lookoutAnchorShoulderLabel(_ value: UInt32) -> String? {
        switch value {
        case 0:
            return "left"
        case 2, 3:
            return "right"
        default:
            return nil
        }
    }

    private static func combatInteractionPrompt(state: MDTBEngineState) -> String? {
        let territoryClaimed =
            state.territory_faction != UInt32(MDTBTerritoryFactionNone) &&
            state.territory_phase != UInt32(MDTBTerritoryPhaseNone)
        let territoryName = territoryFactionLabel(state.territory_faction)
        let territoryVehicleEntry = state.territory_entry_mode == UInt32(MDTBTerritoryEntryVehicle)
        let territorySkimming = territorySkimmingLine(state: state)
        let territoryDeeper = territoryPushingDeeper(state: state)
        let territoryWatch = territoryWatchIntensity(state: state)
        let territoryPatrolWatch = territoryPatrolWatchActive(state: state)
        let territoryPatrolScreen = territoryPatrolScreening(state: state)
        let territoryPatrolClear = territoryPatrolClearing(state: state)
        let territoryPatrolReform = territoryPatrolReforming(state: state)
        let territoryPatrolBrace = territoryPatrolHardening(state: state)
        let territoryPatrolHandoff = state.territory_patrol_state == UInt32(MDTBTerritoryPatrolHandoff)
        let territoryInnerWatch = territoryInnerWatchActive(state: state)
        let territoryEntryClamp = territoryEntryClamped(state: state)
        let territoryCommitWindowOpen = territoryCommitWindowActive(state: state)
        let territoryCommitLive = territoryCommitActive(state: state)
        let territoryCommitResolved = territoryCommitComplete(state: state)
        let territoryResolveWindowOpen = territoryResolveWindowActive(state: state)
        let territoryResolveHoldLive = territoryResolveHoldActive(state: state)
        let territoryResolvePulloutLive = territoryResolvePulloutActive(state: state)
        let territoryResolveProgress = min(max(state.territory_resolve_progress, 0.0), 1.0)
        let territoryResolveHeld = territoryResolveHoldLive && territoryResolveProgress > 0.99
        let territoryResolveClean = territoryResolvePulloutLive && territoryResolveProgress > 0.99
        let territoryReclaimReturn = territoryReclaimReturnActive(state: state)
        let territoryRetakeReturn = territoryRetakeReturnActive(state: state)
        let territoryReclaimFeint = territoryReclaimFeint(state: state)
        let territoryReclaimCommit = territoryReclaimCommit(state: state)
        let territoryReclaimCounter = territoryReclaimCounterStep(state: state)
        let territoryRetakeChallenge = territoryRetakeChallenge(state: state)
        let territoryRetakeCommit = territoryRetakeCommit(state: state)
        let territoryRetakeCross = territoryRetakeCrossAngle(state: state)
        let territoryPreferredShoulder = territoryPreferredShoulderLabel(state: state)
        let anchorLabel = lookoutAnchorLabel(state.combat_hostile_anchor_index)
        let lookoutShoulder = lookoutAnchorShoulderLabel(state.combat_hostile_anchor_index)
        let searchActive = state.combat_hostile_search_timer > 0.0
        let recoveryActive = streetRecoveryIntensity(state: state) > 0.08
        let lateFallback = territoryLateFallbackActive(state: state)
        let approachCarry = territoryApproachCarryActive(state: state)
        let coldWatchCarry = territoryColdWatchCarryActive(state: state)
        let coldStartCarry = territoryColdStartCarryActive(state: state)

        if state.player_reset_timer > 0.0 {
            return "encounter reset \(formatScalar(state.player_reset_timer))s / regroup before re-engaging"
        }

        if let pickupPrompt = pickupPrompt(state: state) {
            return pickupPrompt
        }

        if territoryResolveHeld {
            return territoryPreferredShoulder != nil && lookoutShoulder == territoryPreferredShoulder
                ? "held pocket / \(anchorLabel) is leaning into the \(territoryPreferredShoulder!) shoulder reclaim"
                : (territoryPreferredShoulder != nil
                    ? "held pocket / court-set reclaim is building deeper on the \(territoryPreferredShoulder!) shoulder around you"
                    : "held pocket / court-set reclaim is building deeper around you")
        }

        if territoryResolveClean {
            return searchActive
                ? (territoryPreferredShoulder != nil && lookoutShoulder == territoryPreferredShoulder
                    ? "clean pullout / \(anchorLabel) is checking the \(territoryPreferredShoulder!) shoulder while the reseal closes"
                    : (territoryPreferredShoulder != nil
                        ? "clean pullout / edge retake and exit search are closing on the \(territoryPreferredShoulder!) shoulder behind you"
                        : "clean pullout / edge retake and exit search are closing behind you"))
                : (territoryPreferredShoulder != nil && lookoutShoulder == territoryPreferredShoulder
                    ? "clean pullout / \(anchorLabel) and the edge are sealing the \(territoryPreferredShoulder!) shoulder behind you"
                    : (territoryPreferredShoulder != nil
                        ? "clean pullout / runner and post are resealing the \(territoryPreferredShoulder!) shoulder of the lane mouth behind you"
                        : "clean pullout / runner and post are resealing the lane mouth behind you"))
        }

        if territoryResolveHoldLive {
            return territoryPreferredShoulder != nil
                ? "hold the pocket and force a deeper reclaim beat on the \(territoryPreferredShoulder!) shoulder"
                : "hold the pocket and force a deeper reclaim beat"
        }

        if territoryResolvePulloutLive {
            return territoryPreferredShoulder != nil
                ? "pull out clean now before the \(territoryPreferredShoulder!) shoulder reseal locks in"
                : "pull out clean now before the edge retake locks in"
        }

        if territoryResolveWindowOpen {
            return "line broken / hold the pocket or pull out before the block retakes the edge"
        }

        if territoryReclaimReturn {
            return territoryReclaimFeint
                ? (territoryReclaimCounter
                    ? (territoryPreferredShoulder != nil
                        ? "reclaim feint / runner peels off the \(territoryPreferredShoulder!) shoulder while the post turns the next angle deeper"
                        : "reclaim feint / runner is peeling off the mouth while the post turns the next angle deeper")
                    : "reclaim feint / the edge is soft, but the post is baiting the next commit deeper")
                : (territoryReclaimCommit
                    ? (territoryPreferredShoulder != nil
                        ? "reclaim commit / you took the softer \(territoryPreferredShoulder!) shoulder and the pocket is waking fast there"
                        : "reclaim commit / you took the softer mouth and the pocket is waking fast")
                    : (territoryClaimed
                        ? (territoryPreferredShoulder != nil
                            ? "reclaim return / the \(territoryPreferredShoulder!) shoulder eased off but the pocket is waking deeper on this pass"
                            : "reclaim return / the edge eased off but the pocket is waking deeper on this pass")
                        : "pocket remembered / the runner is lighter at the mouth while the post waits deeper"))
        }

        if territoryRetakeReturn {
            return territoryRetakeChallenge
                ? (territoryRetakeCross
                    ? (territoryPreferredShoulder != nil
                        ? "edge challenge / runner and post are cross-angling the \(territoryPreferredShoulder!) shoulder before the heavier recommit"
                        : "edge challenge / runner and post are cross-angling the mouth before the heavier recommit")
                    : (searchActive
                        ? "edge challenge / the lane mouth is testing you early while exit search still echoes"
                        : "edge challenge / runner and post are checking the return at the mouth before deeper commit"))
                : (territoryRetakeCommit
                    ? (territoryPreferredShoulder != nil
                        ? "retake commit / you hit the \(territoryPreferredShoulder!) shoulder challenge and the block is recommitting through that side"
                        : "retake commit / you hit the challenge and the block is recommitting through the mouth")
                    : (searchActive
                        ? "retake return / the lane mouth is locking up early while exit search still echoes"
                        : (territoryPreferredShoulder != nil
                            ? "edge remembered / runner and post are answering earlier on the \(territoryPreferredShoulder!) shoulder"
                            : "edge remembered / runner and post are answering earlier at the lane mouth")))
        }

        if territoryCommitResolved {
            return "line broken / hold the pocket or pull out on your terms"
        }

        if territoryCommitLive {
            return "push is counting / stay inside the pocket until the beat lands"
        }

        if territoryCommitWindowOpen {
            return "hardened \(territoryName) line / push through now before the window settles"
        }

        if lateFallback {
            if state.traversal_mode == UInt32(MDTBTraversalModeVehicle) {
                return territoryPreferredShoulder != nil && lookoutShoulder == territoryPreferredShoulder
                    ? "late fallback / \(anchorLabel) and the vehicle line are settling back onto the \(territoryPreferredShoulder!) shoulder"
                    : (territoryPreferredShoulder != nil
                        ? "late fallback / court-set is settling the vehicle line back onto the \(territoryPreferredShoulder!) shoulder"
                        : "late fallback / court-set is settling the vehicle line")
            }

            return territoryPreferredShoulder != nil && lookoutShoulder == territoryPreferredShoulder
                ? "late fallback / \(anchorLabel) and the line are settling back onto the \(territoryPreferredShoulder!) shoulder"
                : (territoryPreferredShoulder != nil
                    ? "late fallback / court-set is settling the line back onto the \(territoryPreferredShoulder!) shoulder"
                    : "late fallback / court-set is settling the line")
        }

        if state.traversal_mode == UInt32(MDTBTraversalModeVehicle) {
            guard state.equipped_weapon_kind != UInt32(MDTBEquippedWeaponNone) else {
                if state.territory_phase == UInt32(MDTBTerritoryPhaseHot) {
                    return territoryPreferredShoulder != nil
                        ? "hot \(territoryName) block / vehicle re-entry is being read fast on the \(territoryPreferredShoulder!) shoulder"
                        : "hot \(territoryName) block / vehicle re-entry is being read fast"
                }
                if !territoryClaimed && territoryPatrolWatch {
                    return territoryPatrolHandoff
                        ? "court-set runner is turning the vehicle toward the lane before the line"
                        : (territoryPatrolBrace
                            ? (coldStartCarry && territoryPreferredShoulder != nil
                                ? "court-set line is carrying the last \(territoryPreferredShoulder!) shoulder into a cold vehicle clamp"
                                : "court-set line is hardening again / step back in and the vehicle clamp locks up")
                            : (territoryPatrolReform
                            ? (recoveryActive && territoryPreferredShoulder != nil
                                ? "court-set line is reforming the \(territoryPreferredShoulder!) shoulder while spillback reopens that vehicle lane"
                                : "court-set line is reforming after the clear / feint in again and the vehicle screen closes back up")
                            : (territoryPatrolScreen
                            ? (coldStartCarry && territoryPreferredShoulder != nil
                                ? "court-set runner is carrying the last \(territoryPreferredShoulder!) shoulder into this cold vehicle screen"
                                : "court-set runner is screening the vehicle at the lane mouth / the line is pinching")
                            : (approachCarry && territoryPreferredShoulder != nil
                                ? "court-set runner is drifting into the last \(territoryPreferredShoulder!) shoulder before the outside vehicle watch fully forms"
                                : (coldWatchCarry && territoryPreferredShoulder != nil
                                    ? "court-set runner is carrying the last \(territoryPreferredShoulder!) shoulder into the first outside vehicle watch"
                                    : "court-set runner is posted on the sidewalk ahead / the vehicle is being read early")))))
                }
                if territoryClaimed && territoryWatch > 0.12 {
                    if territoryDeeper {
                        return territoryPatrolClear && territoryInnerWatch
                            ? "inside \(territoryName) turf / runner cleared the line while the inner post owns the handoff"
                            : (territoryInnerWatch
                            ? "vehicle pushed deeper into \(territoryName) turf / inner post is taking the handoff"
                            : "vehicle pushed deeper into \(territoryName) turf / deep watch is building inside")
                    }
                    if territorySkimming || territoryVehicleEntry {
                        return territoryPatrolBrace
                            ? "rolling the \(territoryName) line / runner and post are locking the vehicle edge back into clamp"
                            : territoryEntryClamp
                            ? "rolling the \(territoryName) line / runner and post are clamping the vehicle from opposite angles"
                            : (territoryPatrolReform
                                ? "rolling the \(territoryName) line / runner is reforming the edge while the post eases back toward support"
                                : (territoryPatrolScreen
                            ? "rolling the \(territoryName) line / the runner is screening the vehicle before handoff"
                            : "rolling the \(territoryName) line / front hold is clocking the vehicle"
                            ))
                    }
                    return "inside \(territoryName) turf / the block is reading the vehicle pass"
                }
                if state.territory_phase == UInt32(MDTBTerritoryPhaseBoundary) {
                    return "rolling the \(territoryName) line / stay moving or rearm at the lane pickups"
                }
                return nil
            }
            if state.equipped_weapon_kind == UInt32(MDTBEquippedWeaponPistol) {
                return "pistol stowed while driving"
            }
            return "\(equippedWeaponLabel(state.equipped_weapon_kind)) stowed while driving"
        }

        let switchHint = weaponSwitchHint(state: state)
        let focusLabel = combatTargetLabel(state.combat_focus_target_kind)

        switch state.equipped_weapon_kind {
        case UInt32(MDTBEquippedWeaponLeadPipe):
            switch state.melee_attack_phase {
            case UInt32(MDTBMeleeAttackWindup):
                return "lead pipe windup \(formatScalar(state.melee_attack_timer))s\(switchHint)"
            case UInt32(MDTBMeleeAttackStrike):
                return (state.melee_attack_connected != 0
                    ? "lead pipe strike connected"
                    : "lead pipe strike live") + switchHint
            case UInt32(MDTBMeleeAttackRecovery):
                return "lead pipe recover \(formatScalar(state.melee_attack_timer))s\(switchHint)"
            default:
                if state.combat_hostile_attack_windup > 0.0 {
                    return "lookout firing \(formatScalar(state.combat_hostile_attack_windup))s / use cover before closing\(switchHint)"
                }
                if state.combat_focus_target_kind != UInt32(MDTBCombatTargetNone) {
                    if (state.combat_focus_target_kind == UInt32(MDTBCombatTargetDummy) && state.combat_target_in_range != 0)
                        || (state.combat_focus_target_kind == UInt32(MDTBCombatTargetLookout) && state.combat_hostile_in_range != 0) {
                        return "Space or click to swing at the \(focusLabel)\(switchHint)"
                    }
                    return "close on the \(focusLabel) and swing with Space or click\(switchHint)"
                }
                if state.combat_target_reset_timer > 0.0 || state.combat_hostile_reset_timer > 0.0 {
                    return "combat lane resetting / re-center on a live target\(switchHint)"
                }
                return "close the gap and face a target before swinging\(switchHint)"
            }
        case UInt32(MDTBEquippedWeaponPistol):
            if state.firearm_reloading != 0 {
                return "pistol reload \(formatScalar(state.firearm_reload_timer))s\(switchHint)"
            }
            if state.firearm_clip_ammo == 0 {
                if state.firearm_reserve_ammo > 0 {
                    return "press Y to reload the pistol\(switchHint)"
                }
                return "pistol empty\(switchHint)"
            }
            if state.combat_hostile_attack_windup > 0.0 {
                return "lookout firing \(formatScalar(state.combat_hostile_attack_windup))s / break line of sight or fire first\(switchHint)"
            }
            if state.combat_focus_target_kind != UInt32(MDTBCombatTargetNone) {
                if state.combat_focus_occluded != 0 {
                    return "cover blocks the \(focusLabel) / step clear and fire / Y reload\(switchHint)"
                }
                if state.combat_focus_alignment >= 0.90 {
                    return "Space or click to fire at the \(focusLabel) / Y reload\(switchHint)"
                }
                return "steady the sights on the \(focusLabel) / Space or click to fire / Y reload\(switchHint)"
            }
            if state.combat_player_in_cover != 0 {
                return "cover is holding / step out, fire, then tuck back in / Y reload\(switchHint)"
            }
            return "line up either target / Space or click to fire / Y reload\(switchHint)"
        default:
            if state.melee_weapon_owned != 0 || state.firearm_owned != 0 {
                return "press 1 or 2 to equip a weapon"
            }
            if state.territory_phase == UInt32(MDTBTerritoryPhaseHot) {
                return territoryPreferredShoulder != nil
                    ? "burned \(territoryName) block / the \(territoryPreferredShoulder!) shoulder is still the fast read, so keep moving or rearm at the lane pickups"
                    : "burned \(territoryName) block / keep moving or rearm at the lane pickups"
            }
            if !territoryClaimed && territoryPatrolWatch {
                return territoryPatrolHandoff
                    ? "court-set runner is turning the lane inward before the line"
                    : (territoryPatrolBrace
                        ? (coldStartCarry && territoryPreferredShoulder != nil
                            ? "court-set line is carrying the last \(territoryPreferredShoulder!) shoulder into a cold clamp"
                            : "court-set line is hardening again / step back in and the clamp locks up")
                        : (territoryPatrolReform
                        ? (recoveryActive && territoryPreferredShoulder != nil
                            ? "court-set line is reforming the \(territoryPreferredShoulder!) shoulder while spillback reopens the lane"
                            : "court-set line is reforming after the clear / step back in and the screen closes again")
                        : (territoryPatrolScreen
                        ? (coldStartCarry && territoryPreferredShoulder != nil
                            ? "court-set runner is carrying the last \(territoryPreferredShoulder!) shoulder into this cold screen"
                            : "court-set runner is screening the lane mouth / step in and the deeper post wakes")
                        : (approachCarry && territoryPreferredShoulder != nil
                            ? "court-set runner is drifting into the last \(territoryPreferredShoulder!) shoulder before the outside watch fully forms"
                            : (coldWatchCarry && territoryPreferredShoulder != nil
                                ? "court-set runner is carrying the last \(territoryPreferredShoulder!) shoulder into the first outside watch"
                                : "court-set runner is posted on the sidewalk ahead / the lane mouth is being watched")))))
            }
            if state.territory_phase == UInt32(MDTBTerritoryPhaseBoundary) || (territoryClaimed && territorySkimming && territoryWatch > 0.08) {
                return territoryPatrolBrace
                    ? "at the \(territoryName) line / runner and post are locking the edge back into clamp"
                    : territoryEntryClamp
                    ? "crossing the \(territoryName) line on foot / runner and post are clamping the entry before the line clears"
                    : (territoryPatrolReform
                        ? "at the \(territoryName) line / the runner is reforming the edge while the post eases back in"
                        : (territoryPatrolScreen
                    ? "crossing the \(territoryName) line on foot / runner is screening the entry before handoff"
                    : "crossing the \(territoryName) line on foot / front hold is reading you before combat"
                    ))
            }
            if territoryClaimed && territoryDeeper && territoryWatch > 0.12 {
                return territoryPatrolClear && territoryInnerWatch
                    ? "inside \(territoryName) turf / runner cleared the line while the inner post owns the pocket"
                    : (territoryInnerWatch
                    ? "inside \(territoryName) turf / the inner post is taking the handoff"
                    : "inside \(territoryName) turf / deeper push is drawing a second set of eyes"
                    )
            }
            if territoryClaimed {
                return "inside \(territoryName) turf / the lookout will read you faster here"
            }
            return nil
        }
    }

    private static func pickupPrompt(state: MDTBEngineState) -> String? {
        let actorPosition = currentPlayerPosition(state: state)
        let pipeDistance = sqrt(distanceSquared(
            actorPosition,
            SIMD3<Float>(state.melee_weapon_pickup_position.x, state.melee_weapon_pickup_position.y, state.melee_weapon_pickup_position.z)
        ))
        let pistolDistance = sqrt(distanceSquared(
            actorPosition,
            SIMD3<Float>(state.firearm_pickup_position.x, state.firearm_pickup_position.y, state.firearm_pickup_position.z)
        ))

        var nearestLabel: String?
        var nearestDistance = Float.greatestFiniteMagnitude

        if state.melee_weapon_owned == 0, state.melee_weapon_pickup_in_range != 0, pipeDistance < nearestDistance {
            nearestLabel = "lead pipe"
            nearestDistance = pipeDistance
        }

        if state.firearm_owned == 0, state.firearm_pickup_in_range != 0, pistolDistance < nearestDistance {
            nearestLabel = "pistol"
            nearestDistance = pistolDistance
        }

        guard let nearestLabel else {
            return nil
        }

        return "press T to grab the \(nearestLabel)"
    }

    private static func pistolAmmoSummary(state: MDTBEngineState) -> String {
        "clip \(state.firearm_clip_ammo) / reserve \(state.firearm_reserve_ammo)"
    }

    private static func combatTargetLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBCombatTargetDummy):
            return "dummy"
        case UInt32(MDTBCombatTargetLookout):
            return "lookout"
        default:
            return "target"
        }
    }

    private static func combatFocusSummary(state: MDTBEngineState) -> String? {
        guard state.combat_focus_target_kind != UInt32(MDTBCombatTargetNone) else {
            return nil
        }
        let occlusion = state.combat_focus_occluded != 0 ? " cover" : ""
        return "focus \(combatTargetLabel(state.combat_focus_target_kind)) \(formatScalar(state.combat_focus_distance))m aim \(formatScalar(state.combat_focus_alignment))\(occlusion)"
    }

    private static func playerStatusSummary(state: MDTBEngineState) -> String {
        if state.player_reset_timer > 0.0 {
            return "player reset \(formatScalar(state.player_reset_timer))s"
        }

        var parts = ["player \(formatScalar(state.player_health))hp"]
        parts.append(state.combat_player_in_cover != 0 ? "cover" : "open")

        if state.player_health <= 40.0 {
            parts.append("low")
        }

        if state.player_damage_pulse >= 0.28 {
            parts.append("hit")
        }

        return parts.joined(separator: " ")
    }

    private static func targetStatusSummary(label: String, distance: String, inRange: Bool, health: Float, resetTimer: Float, alert: Float? = nil) -> String {
        var summary: String

        if health <= 0.0 && resetTimer > 0.0 {
            summary = "\(label) reset \(formatScalar(resetTimer))s"
        } else if inRange {
            summary = "\(label) close \(formatScalar(health))hp"
        } else {
            summary = "\(label) \(distance)m \(formatScalar(health))hp"
        }

        if let alert {
            summary += " a\(formatScalar(alert))"
        }

        return summary
    }

    private static func dummyStatusSummary(state: MDTBEngineState, targetDistance: String) -> String {
        targetStatusSummary(
            label: "dummy",
            distance: targetDistance,
            inRange: state.combat_target_in_range != 0,
            health: state.combat_target_health,
            resetTimer: state.combat_target_reset_timer
        )
    }

    private static func lookoutStatusSummary(state: MDTBEngineState, targetDistance: String) -> String {
        var summary = targetStatusSummary(
            label: "lookout",
            distance: targetDistance,
            inRange: state.combat_hostile_in_range != 0,
            health: state.combat_hostile_health,
            resetTimer: state.combat_hostile_reset_timer,
            alert: state.combat_hostile_alert
        )
        summary += " @\(lookoutAnchorLabel(state.combat_hostile_anchor_index))"
        if let preferredShoulder = territoryPreferredShoulderLabel(state: state),
           lookoutAnchorShoulderLabel(state.combat_hostile_anchor_index) == preferredShoulder {
            if state.territory_resolve_state == UInt32(MDTBTerritoryResolveHold) && state.territory_resolve_timer > 0.05 {
                summary += " \(preferredShoulder)-carry"
            } else if state.territory_resolve_state == UInt32(MDTBTerritoryResolvePullout) && state.territory_resolve_timer > 0.05 {
                summary += " \(preferredShoulder)-seal"
            } else if streetRecoveryIntensity(state: state) > 0.08 {
                summary += " \(preferredShoulder)-reopen"
            } else if streetNormalizingIntensity(state: state) > 0.12 {
                summary += " \(preferredShoulder)-settle"
            } else if state.territory_phase == UInt32(MDTBTerritoryPhaseHot) && state.territory_reentry_timer > 0.0 {
                summary += " \(preferredShoulder)-hot"
            } else if territoryLateFallbackActive(state: state) {
                summary += " \(preferredShoulder)-fallback"
            } else if territoryApproachCarryActive(state: state) {
                summary += " \(preferredShoulder)-approach"
            } else if territoryColdWatchCarryActive(state: state) {
                summary += " \(preferredShoulder)-watch"
            } else if territoryColdStartCarryActive(state: state) {
                summary += " \(preferredShoulder)-cold"
            }
        }

        if state.combat_hostile_search_timer > 0.0 {
            summary += " search \(formatScalar(state.combat_hostile_search_timer))s"
        }

        if state.combat_hostile_attack_windup > 0.0 {
            summary += " fire \(formatScalar(state.combat_hostile_attack_windup))s"
        } else if state.combat_hostile_reacquire_timer > 0.0 {
            summary += " reacq \(formatScalar(state.combat_hostile_reacquire_timer))s"
        } else if state.combat_hostile_last_shot_timer > 0.0 {
            summary += state.combat_hostile_last_shot_hit != 0 ? " shot" : " shot cover"
        }

        return summary
    }

    private static func witnessStateLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBWitnessStateInvestigate):
            return "investigate"
        case UInt32(MDTBWitnessStateFlee):
            return "flee"
        case UInt32(MDTBWitnessStateCooldown):
            return "cooldown"
        default:
            return "idle"
        }
    }

    private static func witnessStatusSummary(state: MDTBEngineState) -> String {
        var parts = ["witness \(witnessStateLabel(state.witness_state))"]

        if state.witness_state != UInt32(MDTBWitnessStateIdle) && state.witness_state_timer > 0.0 {
            parts.append("\(formatScalar(state.witness_state_timer))s")
        }

        if state.witness_alert > 0.05 {
            parts.append("a\(formatScalar(state.witness_alert))")
        }

        return parts.joined(separator: " ")
    }

    private static func bystanderStatusSummary(state: MDTBEngineState) -> String {
        var parts = ["bystander \(witnessStateLabel(state.bystander_state))"]

        if state.bystander_state != UInt32(MDTBWitnessStateIdle) && state.bystander_state_timer > 0.0 {
            parts.append("\(formatScalar(state.bystander_state_timer))s")
        }

        if state.bystander_alert > 0.05 {
            parts.append("a\(formatScalar(state.bystander_alert))")
        }

        return parts.joined(separator: " ")
    }

    private static func civilianResponseSourceCount(state: MDTBEngineState) -> Int {
        var count = 0

        if state.witness_state != UInt32(MDTBWitnessStateIdle) || state.witness_alert > 0.05 || state.witness_state_timer > 0.0 {
            count += 1
        }
        if state.bystander_state != UInt32(MDTBWitnessStateIdle) || state.bystander_alert > 0.05 || state.bystander_state_timer > 0.0 {
            count += 1
        }

        return count
    }

    private static func territoryFactionLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBTerritoryFactionCourtSet):
            return "court-set"
        default:
            return "neutral"
        }
    }

    private static func territoryPhaseLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBTerritoryPhaseBoundary):
            return "line"
        case UInt32(MDTBTerritoryPhaseClaimed):
            return "turf"
        case UInt32(MDTBTerritoryPhaseHot):
            return "burned"
        default:
            return "open"
        }
    }

    private static func territoryEntryModeLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBTerritoryEntryVehicle):
            return "roll"
        case UInt32(MDTBTerritoryEntryOnFoot):
            return "foot"
        default:
            return "open"
        }
    }

    private static func territoryPatrolStateLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBTerritoryPatrolWatch):
            return "watch"
        case UInt32(MDTBTerritoryPatrolScreen):
            return "screen"
        case UInt32(MDTBTerritoryPatrolBrace):
            return "brace"
        case UInt32(MDTBTerritoryPatrolHandoff):
            return "handoff"
        case UInt32(MDTBTerritoryPatrolCooldown):
            return "cool"
        case UInt32(MDTBTerritoryPatrolClear):
            return "clear"
        case UInt32(MDTBTerritoryPatrolReform):
            return "reform"
        default:
            return "idle"
        }
    }

    private static func territoryWatchIntensity(state: MDTBEngineState) -> Float {
        max(state.territory_front_watch, state.territory_deep_watch)
    }

    private static func territoryPatrolWatchActive(state: MDTBEngineState) -> Bool {
        let patrolState = state.territory_patrol_state
        return (
            patrolState == UInt32(MDTBTerritoryPatrolWatch) ||
            patrolState == UInt32(MDTBTerritoryPatrolReform) ||
            patrolState == UInt32(MDTBTerritoryPatrolBrace) ||
            patrolState == UInt32(MDTBTerritoryPatrolScreen) ||
            patrolState == UInt32(MDTBTerritoryPatrolHandoff)
        ) && state.territory_patrol_alert > 0.12
    }

    private static func territoryPatrolScreening(state: MDTBEngineState) -> Bool {
        state.territory_patrol_state == UInt32(MDTBTerritoryPatrolScreen) &&
            state.territory_patrol_alert > 0.14
    }

    private static func territoryPatrolClearing(state: MDTBEngineState) -> Bool {
        state.territory_patrol_state == UInt32(MDTBTerritoryPatrolClear) &&
            state.territory_patrol_alert > 0.12
    }

    private static func territoryPatrolReforming(state: MDTBEngineState) -> Bool {
        state.territory_patrol_state == UInt32(MDTBTerritoryPatrolReform) &&
            state.territory_patrol_alert > 0.12
    }

    private static func territoryPatrolHardening(state: MDTBEngineState) -> Bool {
        state.territory_patrol_state == UInt32(MDTBTerritoryPatrolBrace) &&
            state.territory_patrol_alert > 0.12
    }

    private static func territoryCommitStateLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBTerritoryCommitWindow):
            return "window"
        case UInt32(MDTBTerritoryCommitActive):
            return "active"
        case UInt32(MDTBTerritoryCommitComplete):
            return "done"
        default:
            return "idle"
        }
    }

    private static func territoryCommitWindowActive(state: MDTBEngineState) -> Bool {
        state.territory_commit_state == UInt32(MDTBTerritoryCommitWindow) &&
            state.territory_commit_timer > 0.05
    }

    private static func territoryCommitActive(state: MDTBEngineState) -> Bool {
        state.territory_commit_state == UInt32(MDTBTerritoryCommitActive) &&
            state.territory_commit_progress > 0.01
    }

    private static func territoryCommitComplete(state: MDTBEngineState) -> Bool {
        state.territory_commit_state == UInt32(MDTBTerritoryCommitComplete) &&
            state.territory_commit_timer > 0.05
    }

    private static func territoryResolveStateLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBTerritoryResolveWindow):
            return "window"
        case UInt32(MDTBTerritoryResolveHold):
            return "hold"
        case UInt32(MDTBTerritoryResolvePullout):
            return "pull"
        default:
            return "idle"
        }
    }

    private static func territoryReapproachStateLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBTerritoryReapproachReclaim):
            return "reclaim"
        case UInt32(MDTBTerritoryReapproachRetake):
            return "retake"
        default:
            return "idle"
        }
    }

    private static func territoryResolveWindowActive(state: MDTBEngineState) -> Bool {
        state.territory_resolve_state == UInt32(MDTBTerritoryResolveWindow) &&
            state.territory_resolve_timer > 0.05
    }

    private static func territoryResolveHoldActive(state: MDTBEngineState) -> Bool {
        state.territory_resolve_state == UInt32(MDTBTerritoryResolveHold) &&
            state.territory_resolve_timer > 0.05
    }

    private static func territoryResolvePulloutActive(state: MDTBEngineState) -> Bool {
        state.territory_resolve_state == UInt32(MDTBTerritoryResolvePullout) &&
            state.territory_resolve_timer > 0.05
    }

    private static func territoryReclaimReturnActive(state: MDTBEngineState) -> Bool {
        state.territory_reapproach_mode == UInt32(MDTBTerritoryReapproachReclaim) &&
            state.territory_reapproach_timer > 0.05
    }

    private static func territoryRetakeReturnActive(state: MDTBEngineState) -> Bool {
        state.territory_reapproach_mode == UInt32(MDTBTerritoryReapproachRetake) &&
            state.territory_reapproach_timer > 0.05
    }

    private static func territoryReclaimFeint(state: MDTBEngineState) -> Bool {
        territoryReclaimReturnActive(state: state) &&
            territorySkimmingLine(state: state) &&
            territoryPatrolScreening(state: state) &&
            !territoryEntryClamped(state: state) &&
            state.territory_inner_state == UInt32(MDTBTerritoryPatrolHandoff) &&
            state.territory_inner_alert > 0.12
    }

    private static func territoryReclaimCommit(state: MDTBEngineState) -> Bool {
        territoryReclaimReturnActive(state: state) &&
            territoryPushingDeeper(state: state) &&
            (state.territory_patrol_state == UInt32(MDTBTerritoryPatrolHandoff) ||
             state.territory_patrol_state == UInt32(MDTBTerritoryPatrolClear) ||
             state.territory_inner_state == UInt32(MDTBTerritoryPatrolHandoff)) &&
            (state.territory_patrol_alert > 0.12 || state.territory_inner_alert > 0.12)
    }

    private static func territoryRetakeChallenge(state: MDTBEngineState) -> Bool {
        territoryRetakeReturnActive(state: state) &&
            territorySkimmingLine(state: state) &&
            (territoryPatrolHardening(state: state) ||
             territoryPatrolScreening(state: state) ||
             territoryEntryClamped(state: state)) &&
            state.territory_patrol_alert > 0.12
    }

    private static func territoryRetakeCommit(state: MDTBEngineState) -> Bool {
        territoryRetakeReturnActive(state: state) &&
            territoryPushingDeeper(state: state) &&
            (state.territory_patrol_state == UInt32(MDTBTerritoryPatrolBrace) ||
             state.territory_patrol_state == UInt32(MDTBTerritoryPatrolHandoff) ||
             state.territory_inner_state == UInt32(MDTBTerritoryPatrolBrace) ||
             state.territory_inner_state == UInt32(MDTBTerritoryPatrolHandoff)) &&
            (state.territory_patrol_alert > 0.12 || state.territory_inner_alert > 0.12)
    }

    private static func territoryWrappedAngle(_ value: Float) -> Float {
        var wrapped = value
        while wrapped <= -.pi {
            wrapped += (.pi * 2.0)
        }
        while wrapped > .pi {
            wrapped -= (.pi * 2.0)
        }
        return wrapped
    }

    private static func territoryAngleSeparation(_ a: Float, _ b: Float) -> Float {
        abs(territoryWrappedAngle(a - b))
    }

    private static func territoryPreferredSideBias(state: MDTBEngineState) -> Float {
        max(-1.0, min(state.territory_preferred_side, 1.0))
    }

    private static func territoryPreferredShoulderLabel(state: MDTBEngineState) -> String? {
        let bias = territoryPreferredSideBias(state: state)
        guard abs(bias) > 0.22 else {
            return nil
        }
        return bias < 0.0 ? "left" : "right"
    }

    private static func territoryPreferredShoulderTag(state: MDTBEngineState) -> String? {
        guard let shoulder = territoryPreferredShoulderLabel(state: state) else {
            return nil
        }
        return shoulder == "left" ? "sideL" : "sideR"
    }

    private static func territoryLateFallbackActive(state: MDTBEngineState) -> Bool {
        guard territoryPreferredShoulderLabel(state: state) != nil else {
            return false
        }

        let boundaryOrOpen =
            state.territory_phase == UInt32(MDTBTerritoryPhaseNone) ||
            state.territory_phase == UInt32(MDTBTerritoryPhaseBoundary)
        let patrolFallback =
            state.territory_patrol_state == UInt32(MDTBTerritoryPatrolCooldown) ||
            state.territory_patrol_state == UInt32(MDTBTerritoryPatrolWatch) ||
            state.territory_patrol_state == UInt32(MDTBTerritoryPatrolReform) ||
            state.territory_inner_state == UInt32(MDTBTerritoryPatrolCooldown) ||
            state.territory_inner_state == UInt32(MDTBTerritoryPatrolReform)

        return boundaryOrOpen &&
            state.territory_phase != UInt32(MDTBTerritoryPhaseHot) &&
            state.territory_watch_timer > 0.0 &&
            state.territory_heat > 0.04 &&
            streetRecoveryIntensity(state: state) <= 0.08 &&
            streetNormalizingIntensity(state: state) <= 0.12 &&
            patrolFallback
    }

    private static func territoryColdStartCarryActive(state: MDTBEngineState) -> Bool {
        guard territoryPreferredShoulderLabel(state: state) != nil else {
            return false
        }

        let boundaryApproach =
            state.territory_phase == UInt32(MDTBTerritoryPhaseNone) ||
            state.territory_phase == UInt32(MDTBTerritoryPhaseBoundary)
        let coldCommitBeat =
            territoryPatrolScreening(state: state) ||
            territoryPatrolHardening(state: state) ||
            state.territory_inner_state == UInt32(MDTBTerritoryPatrolBrace)

        return boundaryApproach &&
            !territoryLateFallbackActive(state: state) &&
            state.territory_phase != UInt32(MDTBTerritoryPhaseHot) &&
            state.territory_reentry_timer <= 0.0 &&
            state.territory_reapproach_timer <= 0.05 &&
            state.territory_resolve_timer <= 0.05 &&
            state.combat_hostile_search_timer <= 0.0 &&
            streetRecoveryIntensity(state: state) <= 0.08 &&
            streetNormalizingIntensity(state: state) <= 0.12 &&
            state.territory_watch_timer > 0.0 &&
            state.territory_heat > 0.04 &&
            state.territory_heat < 0.24 &&
            coldCommitBeat &&
            !territoryReclaimReturnActive(state: state) &&
            !territoryRetakeReturnActive(state: state)
    }

    private static func territoryApproachCarryActive(state: MDTBEngineState) -> Bool {
        guard territoryPreferredShoulderLabel(state: state) != nil else {
            return false
        }

        let farApproachWatch =
            state.territory_phase == UInt32(MDTBTerritoryPhaseNone) &&
            state.territory_patrol_state == UInt32(MDTBTerritoryPatrolWatch) &&
            state.territory_patrol_alert > 0.08 &&
            state.territory_inner_state != UInt32(MDTBTerritoryPatrolBrace) &&
            state.territory_inner_state != UInt32(MDTBTerritoryPatrolHandoff) &&
            state.territory_front_watch < 0.12 &&
            state.territory_heat < 0.14

        return farApproachWatch &&
            !territoryLateFallbackActive(state: state) &&
            !territoryColdStartCarryActive(state: state) &&
            state.territory_phase != UInt32(MDTBTerritoryPhaseHot) &&
            state.territory_reentry_timer <= 0.0 &&
            state.territory_reapproach_timer <= 0.05 &&
            state.territory_resolve_timer <= 0.05 &&
            state.combat_hostile_search_timer <= 0.0 &&
            streetRecoveryIntensity(state: state) <= 0.08 &&
            streetNormalizingIntensity(state: state) <= 0.12 &&
            state.territory_watch_timer > 0.0 &&
            state.territory_heat > 0.03 &&
            !territoryReclaimReturnActive(state: state) &&
            !territoryRetakeReturnActive(state: state)
    }

    private static func territoryColdWatchCarryActive(state: MDTBEngineState) -> Bool {
        guard territoryPreferredShoulderLabel(state: state) != nil else {
            return false
        }

        let neutralOutsideWatch =
            state.territory_phase == UInt32(MDTBTerritoryPhaseNone) &&
            state.territory_patrol_state == UInt32(MDTBTerritoryPatrolWatch) &&
            state.territory_patrol_alert > 0.08 &&
            state.territory_inner_state != UInt32(MDTBTerritoryPatrolBrace) &&
            state.territory_inner_state != UInt32(MDTBTerritoryPatrolHandoff)

        return neutralOutsideWatch &&
            !territoryLateFallbackActive(state: state) &&
            !territoryApproachCarryActive(state: state) &&
            !territoryColdStartCarryActive(state: state) &&
            state.territory_phase != UInt32(MDTBTerritoryPhaseHot) &&
            state.territory_reentry_timer <= 0.0 &&
            state.territory_reapproach_timer <= 0.05 &&
            state.territory_resolve_timer <= 0.05 &&
            state.combat_hostile_search_timer <= 0.0 &&
            streetRecoveryIntensity(state: state) <= 0.08 &&
            streetNormalizingIntensity(state: state) <= 0.12 &&
            state.territory_watch_timer > 0.0 &&
            state.territory_heat > 0.03 &&
            state.territory_heat < 0.18 &&
            !territoryReclaimReturnActive(state: state) &&
            !territoryRetakeReturnActive(state: state)
    }

    private static func territoryReclaimCounterStep(state: MDTBEngineState) -> Bool {
        territoryReclaimFeint(state: state) &&
            abs(state.territory_patrol_position.x - state.territory_inner_position.x) > 1.0 &&
            territoryAngleSeparation(state.territory_patrol_heading, state.territory_inner_heading) > 0.45
    }

    private static func territoryRetakeCrossAngle(state: MDTBEngineState) -> Bool {
        territoryRetakeChallenge(state: state) &&
            abs(state.territory_patrol_position.x - state.territory_inner_position.x) > 0.9 &&
            territoryAngleSeparation(state.territory_patrol_heading, state.territory_inner_heading) > 0.60
    }

    private static func territoryInnerWatchActive(state: MDTBEngineState) -> Bool {
        let innerState = state.territory_inner_state
        return (
            innerState == UInt32(MDTBTerritoryPatrolWatch) ||
            innerState == UInt32(MDTBTerritoryPatrolReform) ||
            innerState == UInt32(MDTBTerritoryPatrolBrace) ||
            innerState == UInt32(MDTBTerritoryPatrolHandoff)
        ) && state.territory_inner_alert > 0.12
    }

    private static func territoryEntryClamped(state: MDTBEngineState) -> Bool {
        territorySkimmingLine(state: state) &&
            (territoryPatrolScreening(state: state) || territoryPatrolHardening(state: state)) &&
            territoryInnerWatchActive(state: state) &&
            state.territory_inner_alert > 0.12
    }

    private static func territorySkimmingLine(state: MDTBEngineState) -> Bool {
        guard state.territory_faction != UInt32(MDTBTerritoryFactionNone) else {
            return false
        }

        if state.territory_phase == UInt32(MDTBTerritoryPhaseBoundary) {
            return true
        }

        return state.territory_presence < 0.62 &&
            state.territory_deep_watch <= (state.territory_front_watch + 0.08)
    }

    private static func territoryPushingDeeper(state: MDTBEngineState) -> Bool {
        guard state.territory_faction != UInt32(MDTBTerritoryFactionNone) else {
            return false
        }

        return !territorySkimmingLine(state: state) &&
            (state.territory_presence >= 0.62 ||
             state.territory_deep_watch > (state.territory_front_watch + 0.08))
    }

    private static func territoryStatusSummary(state: MDTBEngineState) -> String? {
        let territoryPreferredShoulder = territoryPreferredShoulderLabel(state: state)
        let territoryShoulderTag = territoryPreferredShoulderTag(state: state)
        let normalizing = streetNormalizingIntensity(state: state) > 0.12
        let recovering = streetRecoveryIntensity(state: state) > 0.08
        let lateFallback = territoryLateFallbackActive(state: state)
        let approachCarry = territoryApproachCarryActive(state: state)
        let coldWatchCarry = territoryColdWatchCarryActive(state: state)
        let coldStartCarry = territoryColdStartCarryActive(state: state)

        guard state.territory_faction != UInt32(MDTBTerritoryFactionNone) ||
                state.territory_heat > 0.05 ||
                state.territory_reentry_timer > 0.0 ||
                state.territory_reapproach_mode != UInt32(MDTBTerritoryReapproachNone) ||
                state.territory_reapproach_timer > 0.05 ||
                state.territory_resolve_state != UInt32(MDTBTerritoryResolveNone) ||
                state.territory_resolve_progress > 0.05 ||
                state.territory_patrol_state != UInt32(MDTBTerritoryPatrolIdle) ||
                state.territory_patrol_alert > 0.05 ||
                state.territory_inner_state != UInt32(MDTBTerritoryPatrolIdle) ||
                state.territory_inner_alert > 0.05 ||
                normalizing ||
                recovering else {
            return nil
        }

        var parts: [String] = []

        if state.territory_faction != UInt32(MDTBTerritoryFactionNone) {
            parts.append(territoryFactionLabel(state.territory_faction))
            parts.append(territoryPhaseLabel(state.territory_phase))
            if state.territory_entry_mode != UInt32(MDTBTerritoryEntryNone) {
                parts.append(territoryEntryModeLabel(state.territory_entry_mode))
            }
            if territorySkimmingLine(state: state) {
                parts.append("edge")
            } else if territoryPushingDeeper(state: state) {
                parts.append("deep")
            }
        }

        if state.territory_presence > 0.05 {
            parts.append("p\(formatScalar(state.territory_presence))")
        }

        if state.territory_heat > 0.05 {
            parts.append("h\(formatScalar(state.territory_heat))")
        }

        if state.territory_watch_timer > 0.0 {
            parts.append("w\(formatScalar(state.territory_watch_timer))s")
        }
        if state.territory_front_watch > 0.05 {
            parts.append("f\(formatScalar(state.territory_front_watch))")
        }
        if state.territory_deep_watch > 0.05 {
            parts.append("d\(formatScalar(state.territory_deep_watch))")
        }
        if state.territory_reentry_timer > 0.0 {
            parts.append("r\(formatScalar(state.territory_reentry_timer))s")
        }
        if state.territory_phase == UInt32(MDTBTerritoryPhaseHot) {
            if let territoryShoulderTag {
                parts.append(territoryShoulderTag)
            }
            parts.append("hot")
        }
        if normalizing {
            if let territoryShoulderTag {
                parts.append(territoryShoulderTag)
            }
            parts.append("settle")
        }
        if recovering {
            if let territoryShoulderTag {
                parts.append(territoryShoulderTag)
            }
            parts.append("reopen")
        }
        if lateFallback {
            if let territoryShoulderTag {
                parts.append(territoryShoulderTag)
            }
            parts.append("fallback")
        }
        if approachCarry {
            if let territoryShoulderTag {
                parts.append(territoryShoulderTag)
            }
            parts.append("approach")
        }
        if coldWatchCarry {
            if let territoryShoulderTag {
                parts.append(territoryShoulderTag)
            }
            parts.append("prewatch")
        }
        if coldStartCarry {
            if let territoryShoulderTag {
                parts.append(territoryShoulderTag)
            }
            parts.append("cold")
        }
        if state.territory_reapproach_mode != UInt32(MDTBTerritoryReapproachNone) || state.territory_reapproach_timer > 0.05 {
            parts.append("return")
            parts.append(territoryReapproachStateLabel(state.territory_reapproach_mode))
            if let territoryShoulderTag {
                parts.append(territoryShoulderTag)
            }
            if territoryReclaimFeint(state: state) {
                parts.append(territoryReclaimCounterStep(state: state) ? "counter" : "feint")
            } else if territoryReclaimCommit(state: state) {
                parts.append("deep")
                if territoryPreferredShoulder != nil {
                    parts.append("carry")
                }
            } else if territoryRetakeChallenge(state: state) {
                parts.append(territoryRetakeCrossAngle(state: state) ? "angle" : "challenge")
            } else if territoryRetakeCommit(state: state) {
                parts.append("recommit")
                if territoryPreferredShoulder != nil {
                    parts.append("seal")
                }
            }
            if state.territory_reapproach_timer > 0.0 {
                parts.append("t\(formatScalar(state.territory_reapproach_timer))s")
            }
        }
        if state.territory_commit_state != UInt32(MDTBTerritoryCommitNone) || state.territory_commit_progress > 0.05 {
            parts.append("commit")
            parts.append(territoryCommitStateLabel(state.territory_commit_state))
            if state.territory_commit_timer > 0.0 {
                parts.append("t\(formatScalar(state.territory_commit_timer))s")
            }
            if state.territory_commit_progress > 0.05 {
                parts.append("p\(formatScalar(state.territory_commit_progress))")
            }
        }
        if state.territory_resolve_state != UInt32(MDTBTerritoryResolveNone) || state.territory_resolve_progress > 0.05 {
            parts.append("resolve")
            parts.append(territoryResolveStateLabel(state.territory_resolve_state))
            if let territoryShoulderTag,
               state.territory_resolve_state == UInt32(MDTBTerritoryResolveHold) || state.territory_resolve_state == UInt32(MDTBTerritoryResolvePullout) {
                parts.append(territoryShoulderTag)
                parts.append(state.territory_resolve_state == UInt32(MDTBTerritoryResolveHold) ? "carry" : "seal")
            }
            if state.territory_resolve_timer > 0.0 {
                parts.append("t\(formatScalar(state.territory_resolve_timer))s")
            }
            if state.territory_resolve_progress > 0.05 {
                parts.append("p\(formatScalar(state.territory_resolve_progress))")
            }
        }
        if state.territory_patrol_state != UInt32(MDTBTerritoryPatrolIdle) || state.territory_patrol_alert > 0.05 {
            parts.append("patrol")
            parts.append(territoryPatrolStateLabel(state.territory_patrol_state))
            if state.territory_patrol_alert > 0.05 {
                parts.append("a\(formatScalar(state.territory_patrol_alert))")
            }
        }
        if state.territory_inner_state != UInt32(MDTBTerritoryPatrolIdle) || state.territory_inner_alert > 0.05 {
            parts.append("post")
            parts.append(territoryPatrolStateLabel(state.territory_inner_state))
            if state.territory_inner_alert > 0.05 {
                parts.append("a\(formatScalar(state.territory_inner_alert))")
            }
        }

        return "terr " + parts.joined(separator: " ")
    }

    private static func systemicPressureSummary(state: MDTBEngineState) -> String {
        var parts: [String] = []
        let normalizing = streetNormalizingIntensity(state: state) > 0.12
        let recovering = streetRecoveryIntensity(state: state) > 0.08
        let territoryShoulder = territoryPreferredShoulderLabel(state: state).map { "\($0)-shoulder" }

        if state.witness_state != UInt32(MDTBWitnessStateIdle) || state.witness_alert > 0.05 || state.witness_state_timer > 0.0 {
            parts.append(witnessStatusSummary(state: state))
        }

        if state.bystander_state != UInt32(MDTBWitnessStateIdle) || state.bystander_alert > 0.05 || state.bystander_state_timer > 0.0 {
            parts.append(bystanderStatusSummary(state: state))
        }

        if state.combat_hostile_search_timer > 0.0 {
            parts.append("search \(formatScalar(state.combat_hostile_search_timer))s")
        }

        if state.street_incident_timer > 0.0 && state.street_incident_level > 0.05 {
            parts.append("incident \(formatScalar(state.street_incident_level)) \(formatScalar(state.street_incident_timer))s")
        }

        if normalizing {
            parts.append(territoryShoulder != nil ? "\(territoryShoulder!) normalizing" : "normalizing")
        }

        if recovering {
            parts.append(
                territoryShoulder != nil
                    ? "\(territoryShoulder!) reopening \(formatScalar(state.street_recovery_timer))s"
                    : "reopening \(formatScalar(state.street_recovery_timer))s"
            )
        }

        if let territorySummary = territoryStatusSummary(state: state) {
            parts.append(territorySummary)
        }

        if parts.isEmpty {
            return "street calm"
        }

        return "street " + parts.joined(separator: " / ")
    }

    private static func systemicHUDStatus(state: MDTBEngineState, trafficHazard: Float) -> (title: String, detail: String) {
        let searchActive = state.combat_hostile_search_timer > 0.0
        let searchTimer = formatScalar(state.combat_hostile_search_timer)
        let witnessTimer = formatScalar(state.witness_state_timer)
        let incidentActive = state.street_incident_timer > 0.0 && state.street_incident_level > 0.08
        let incidentTimer = formatScalar(state.street_incident_timer)
        let bystanderTimer = formatScalar(state.bystander_state_timer)
        let civilianSources = civilianResponseSourceCount(state: state)
        let normalizing = streetNormalizingIntensity(state: state) > 0.12
        let recovering = streetRecoveryIntensity(state: state) > 0.08
        let recoveryTimer = formatScalar(state.street_recovery_timer)
        let territoryClaimed =
            state.territory_faction != UInt32(MDTBTerritoryFactionNone) &&
            state.territory_phase != UInt32(MDTBTerritoryPhaseNone)
        let territoryName = territoryFactionLabel(state.territory_faction)
        let territoryTimer = formatScalar(state.territory_reentry_timer)
        let territoryVehicleEntry = state.territory_entry_mode == UInt32(MDTBTerritoryEntryVehicle)
        let territorySkimming = territorySkimmingLine(state: state)
        let territoryDeeper = territoryPushingDeeper(state: state)
        let territoryWatch = territoryWatchIntensity(state: state)
        let territoryPatrolWatch = territoryPatrolWatchActive(state: state)
        let territoryPatrolScreen = territoryPatrolScreening(state: state)
        let territoryPatrolClear = territoryPatrolClearing(state: state)
        let territoryPatrolReform = territoryPatrolReforming(state: state)
        let territoryPatrolBrace = territoryPatrolHardening(state: state)
        let territoryPatrolHandoff = state.territory_patrol_state == UInt32(MDTBTerritoryPatrolHandoff)
        let territoryInnerWatch = territoryInnerWatchActive(state: state)
        let territoryEntryClamp = territoryEntryClamped(state: state)
        let territoryCommitWindowOpen = territoryCommitWindowActive(state: state)
        let territoryCommitLive = territoryCommitActive(state: state)
        let territoryCommitResolved = territoryCommitComplete(state: state)
        let territoryCommitProgress = min(max(state.territory_commit_progress, 0.0), 1.0)
        let territoryResolveWindowOpen = territoryResolveWindowActive(state: state)
        let territoryResolveHoldLive = territoryResolveHoldActive(state: state)
        let territoryResolvePulloutLive = territoryResolvePulloutActive(state: state)
        let territoryResolveProgress = min(max(state.territory_resolve_progress, 0.0), 1.0)
        let territoryResolveHeld = territoryResolveHoldLive && territoryResolveProgress > 0.99
        let territoryResolveClean = territoryResolvePulloutLive && territoryResolveProgress > 0.99
        let territoryReclaimReturn = territoryReclaimReturnActive(state: state)
        let territoryRetakeReturn = territoryRetakeReturnActive(state: state)
        let territoryReclaimFeint = territoryReclaimFeint(state: state)
        let territoryReclaimCommit = territoryReclaimCommit(state: state)
        let territoryReclaimCounter = territoryReclaimCounterStep(state: state)
        let territoryRetakeChallenge = territoryRetakeChallenge(state: state)
        let territoryRetakeCommit = territoryRetakeCommit(state: state)
        let territoryRetakeCross = territoryRetakeCrossAngle(state: state)
        let territoryPreferredShoulder = territoryPreferredShoulderLabel(state: state)
        let territoryShoulder = territoryPreferredShoulder.map { "\($0) shoulder" }
        let anchorLabel = lookoutAnchorLabel(state.combat_hostile_anchor_index)
        let lookoutShoulder = lookoutAnchorShoulderLabel(state.combat_hostile_anchor_index)
        let territoryReapproachTimer = formatScalar(state.territory_reapproach_timer)
        let lateFallback = territoryLateFallbackActive(state: state)
        let approachCarry = territoryApproachCarryActive(state: state)
        let coldWatchCarry = territoryColdWatchCarryActive(state: state)
        let coldStartCarry = territoryColdStartCarryActive(state: state)

        if normalizing {
            return (
                "Street normalizing",
                civilianSources > 0
                    ? (territoryShoulder != nil
                        ? "Civilians are settling while traffic eases off the \(territoryShoulder!) \(incidentTimer)s"
                        : "Civilians are settling while traffic returns to the lane \(incidentTimer)s")
                    : (territoryShoulder != nil
                        ? "Traffic and civilian spillover are easing off the \(territoryShoulder!) of the block \(incidentTimer)s"
                        : "Traffic and civilian spillover are easing off the block \(incidentTimer)s")
            )
        }

        if recovering {
            return (
                "Street reopening",
                territoryPatrolReform && territoryShoulder != nil
                    ? "Traffic is reopening the \(territoryShoulder!) while the boundary reforms there \(recoveryTimer)s"
                    : (civilianSources > 0
                        ? (territoryShoulder != nil
                            ? "Civilians are drifting back while traffic reopens the \(territoryShoulder!) \(recoveryTimer)s"
                            : "Civilians are drifting back while traffic retakes the block \(recoveryTimer)s")
                        : (territoryShoulder != nil
                            ? "Traffic is reopening the \(territoryShoulder!) and nearby link \(recoveryTimer)s"
                            : "Traffic is reopening the lane and nearby link \(recoveryTimer)s"))
            )
        }

        if state.territory_phase == UInt32(MDTBTerritoryPhaseHot) {
            return (
                "Burned territory",
                searchActive
                    ? (territoryShoulder != nil
                        ? "\(territoryName) block remembers the last \(territoryShoulder!) pass / lookout search \(searchTimer)s"
                        : "\(territoryName) block remembers the last pass / lookout search \(searchTimer)s")
                    : (territoryShoulder != nil
                        ? "You re-entered a hot \(territoryName) block / the \(territoryShoulder!) is being read early \(territoryTimer)s"
                        : "You re-entered a hot \(territoryName) block / the lookout is already keyed up \(territoryTimer)s")
            )
        }

        if lateFallback {
            return (
                "Line settling",
                lookoutShoulder == territoryPreferredShoulder && territoryPreferredShoulder != nil
                    ? "\(anchorLabel) is hanging on the \(territoryPreferredShoulder!) shoulder while the line settles there"
                    : (territoryShoulder != nil
                        ? "The block is settling back onto the \(territoryShoulder!) after the reopen"
                        : "The block is settling after the reopen")
            )
        }

        if approachCarry && !territoryClaimed && territoryPatrolWatch && !incidentActive && !searchActive {
            return (
                "Outer post carry",
                state.traversal_mode == UInt32(MDTBTraversalModeVehicle)
                    ? (lookoutShoulder == territoryPreferredShoulder && territoryPreferredShoulder != nil
                        ? "\(anchorLabel) is drifting onto the \(territoryPreferredShoulder!) shoulder before the outside vehicle watch fully forms"
                        : (territoryShoulder != nil
                            ? "The first outer-post vehicle drift is leaning onto the \(territoryShoulder!) before the outside watch fully forms"
                            : "The first outer-post vehicle drift is leaning before the outside watch fully forms"))
                    : (lookoutShoulder == territoryPreferredShoulder && territoryPreferredShoulder != nil
                        ? "\(anchorLabel) is drifting onto the \(territoryPreferredShoulder!) shoulder before the outside watch fully forms"
                        : (territoryShoulder != nil
                            ? "The first outer-post drift is leaning onto the \(territoryShoulder!) before the outside watch fully forms"
                            : "The first outer-post drift is leaning before the outside watch fully forms"))
            )
        }

        if coldWatchCarry && !territoryClaimed && territoryPatrolWatch && !incidentActive && !searchActive {
            return (
                "Outside watch carry",
                state.traversal_mode == UInt32(MDTBTraversalModeVehicle)
                    ? (lookoutShoulder == territoryPreferredShoulder && territoryPreferredShoulder != nil
                        ? "\(anchorLabel) is carrying the \(territoryPreferredShoulder!) shoulder into the first outside vehicle watch"
                        : (territoryShoulder != nil
                            ? "The first outside vehicle watch is leaning onto the \(territoryShoulder!) before the screen forms"
                            : "The first outside vehicle watch is leaning before the screen forms"))
                    : (lookoutShoulder == territoryPreferredShoulder && territoryPreferredShoulder != nil
                        ? "\(anchorLabel) is carrying the \(territoryPreferredShoulder!) shoulder into the first outside watch"
                        : (territoryShoulder != nil
                            ? "The first outside watch is leaning onto the \(territoryShoulder!) before the screen forms"
                            : "The first outside watch is leaning before the screen forms"))
            )
        }

        if coldStartCarry && !territoryClaimed && territoryPatrolWatch && !incidentActive && !searchActive {
            return (
                "Cold shoulder carry",
                state.traversal_mode == UInt32(MDTBTraversalModeVehicle)
                    ? (territoryPatrolBrace
                        ? (territoryShoulder != nil
                            ? "The first cold vehicle clamp is locking on the \(territoryShoulder!) instead of starting from center"
                            : "The first cold vehicle clamp is locking before the lane can reset")
                        : (territoryPatrolScreen
                            ? (territoryShoulder != nil
                                ? "The first cold vehicle screen is forming on the \(territoryShoulder!) instead of starting from center"
                                : "The first cold vehicle screen is forming before the lane can reset")
                            : (lookoutShoulder == territoryPreferredShoulder && territoryPreferredShoulder != nil
                                ? "\(anchorLabel) is carrying the \(territoryPreferredShoulder!) shoulder into the first cold vehicle read"
                                : (territoryShoulder != nil
                                    ? "The first cold vehicle read is leaning onto the \(territoryShoulder!) instead of starting from center"
                                    : "The first cold vehicle read is leaning before the lane can reset"))))
                    : (territoryPatrolBrace
                        ? (territoryShoulder != nil
                            ? "The first cold clamp is locking on the \(territoryShoulder!) instead of starting from center"
                            : "The first cold clamp is locking before the lane can reset")
                        : (territoryPatrolScreen
                            ? (territoryShoulder != nil
                                ? "The first cold screen is forming on the \(territoryShoulder!) instead of starting from center"
                                : "The first cold screen is forming before the lane can reset")
                            : (lookoutShoulder == territoryPreferredShoulder && territoryPreferredShoulder != nil
                                ? "\(anchorLabel) is carrying the \(territoryPreferredShoulder!) shoulder into the first cold read"
                                : (territoryShoulder != nil
                                    ? "The first cold read is leaning onto the \(territoryShoulder!) instead of starting from center"
                                    : "The first cold read is leaning before the lane can reset"))))
            )
        }

        if territoryResolveHeld {
            return (
                "Pocket reclaim",
                lookoutShoulder == territoryPreferredShoulder && territoryPreferredShoulder != nil
                    ? "You held the break / \(anchorLabel) is leaning into the \(territoryPreferredShoulder!) shoulder while reclaim deepens \(formatScalar(state.territory_resolve_timer))s"
                    : (territoryPreferredShoulder != nil
                        ? "You held the break / runner and post are reclaiming deeper turf on the \(territoryPreferredShoulder!) shoulder \(formatScalar(state.territory_resolve_timer))s"
                        : "You held the break / runner and post are reclaiming deeper turf \(formatScalar(state.territory_resolve_timer))s")
            )
        }

        if territoryResolveClean {
            return (
                "Edge retaking",
                searchActive
                    ? (lookoutShoulder == territoryPreferredShoulder && territoryPreferredShoulder != nil
                        ? "You pulled out clean / \(anchorLabel) is checking the \(territoryPreferredShoulder!) shoulder while it reseals \(searchTimer)s"
                        : (territoryPreferredShoulder != nil
                            ? "You pulled out clean / the lookout is checking your exit while the \(territoryPreferredShoulder!) shoulder reseals \(searchTimer)s"
                            : "You pulled out clean / the lookout is checking your exit while the edge reseals \(searchTimer)s"))
                    : (lookoutShoulder == territoryPreferredShoulder && territoryPreferredShoulder != nil
                        ? "You pulled out clean / \(anchorLabel) and the edge are sealing the \(territoryPreferredShoulder!) shoulder \(formatScalar(state.territory_resolve_timer))s"
                        : (territoryPreferredShoulder != nil
                            ? "You pulled out clean / runner and post are resealing the \(territoryPreferredShoulder!) shoulder of the lane mouth \(formatScalar(state.territory_resolve_timer))s"
                            : "You pulled out clean / runner and post are resealing the lane mouth \(formatScalar(state.territory_resolve_timer))s"))
            )
        }

        if territoryResolveHoldLive {
            return (
                "Pocket reclaim live",
                "Stay inside the pocket \(formatScalar(state.territory_resolve_timer))s / progress \(formatScalar(territoryResolveProgress))"
            )
        }

        if territoryResolvePulloutLive {
            return (
                "Edge retake live",
                "Back through the lane mouth \(formatScalar(state.territory_resolve_timer))s / progress \(formatScalar(territoryResolveProgress))"
            )
        }

        if territoryResolveWindowOpen {
            return (
                "Resolve beat",
                "Hold the pocket or pull out before the edge retakes \(formatScalar(state.territory_resolve_timer))s"
            )
        }

        if territoryReclaimReturn {
            return (
                territoryReclaimFeint ? "Reclaim feint" : (territoryReclaimCommit ? "Reclaim commit" : (territoryClaimed ? "Reclaim return" : "Pocket remembered")),
                territoryReclaimFeint
                    ? (territoryReclaimCounter
                        ? (territoryPreferredShoulder != nil
                            ? "Runner is counter-stepping off the \(territoryPreferredShoulder!) shoulder while the post turns the next deeper angle \(territoryReapproachTimer)s"
                            : "Runner is counter-stepping off the mouth while the post turns the next deeper angle \(territoryReapproachTimer)s")
                        : "The mouth is showing soft, but the post is baiting the next deeper pocket \(territoryReapproachTimer)s")
                    : (territoryReclaimCommit
                        ? (territoryPreferredShoulder != nil
                            ? "You took the softer \(territoryPreferredShoulder!) shoulder / runner clears there while the deeper handoff snaps up \(territoryReapproachTimer)s"
                            : "You took the softer edge / runner clears while the deeper handoff snaps up \(territoryReapproachTimer)s")
                        : (territorySkimming
                            ? (territoryPreferredShoulder != nil
                                ? "The \(territoryPreferredShoulder!) shoulder eased off, but the post and lookout are reading the deeper pocket \(territoryReapproachTimer)s"
                                : "The lane mouth eased off, but the post and lookout are reading the deeper pocket \(territoryReapproachTimer)s")
                            : "The last hold still lives in the block / runner clears lighter while the pocket wakes faster \(territoryReapproachTimer)s"))
            )
        }

        if territoryRetakeReturn {
            return (
                territoryRetakeChallenge ? "Edge challenge" : (territoryRetakeCommit ? "Retake commit" : (territorySkimming ? "Retake return" : "Edge remembered")),
                territoryRetakeChallenge
                    ? (territoryRetakeCross
                        ? (territoryPreferredShoulder != nil
                            ? "Runner and post are cross-angling the \(territoryPreferredShoulder!) shoulder before the heavier recommit \(territoryReapproachTimer)s"
                            : "Runner and post are cross-angling the lane mouth before the heavier recommit \(territoryReapproachTimer)s")
                        : (searchActive
                            ? "The clean exit is still echoing / the lane mouth is testing the return with search \(searchTimer)s"
                            : "The last pullout still sits on the edge / runner and post are challenging the mouth early \(territoryReapproachTimer)s"))
                    : (territoryRetakeCommit
                        ? (territoryPreferredShoulder != nil
                            ? "You pushed through the early \(territoryPreferredShoulder!) shoulder challenge / the block is recommitting through that side \(territoryReapproachTimer)s"
                            : "You pushed through the early challenge / the block is recommitting through the mouth \(territoryReapproachTimer)s")
                        : (searchActive
                            ? "The clean exit is still echoing / the lane mouth is locking back up with search \(searchTimer)s"
                            : (territoryPreferredShoulder != nil
                                ? "The last pullout still sits on the \(territoryPreferredShoulder!) shoulder / runner and post are answering earlier \(territoryReapproachTimer)s"
                                : "The last pullout still sits on the edge / runner and post are answering earlier \(territoryReapproachTimer)s")))
            )
        }

        if territoryCommitResolved {
            return (
                "Line broken",
                "You pushed through the hardened \(territoryName) edge / pocket settling \(formatScalar(state.territory_commit_timer))s"
            )
        }

        if territoryCommitLive {
            return (
                "Commit live",
                "Hold inside the pocket \(formatScalar(state.territory_commit_timer))s / progress \(formatScalar(territoryCommitProgress))"
            )
        }

        if territoryCommitWindowOpen {
            return (
                "Commit window",
                "Push through the hardened line before it settles \(formatScalar(state.territory_commit_timer))s"
            )
        }

        if !territoryClaimed && territoryPatrolWatch && !incidentActive && !searchActive {
            return (
                territoryPatrolBrace ? "Line hardening" : (territoryPatrolReform ? "Line reforming" : (territoryPatrolScreen ? "Entry screened" : "Sidewalk watched")),
                state.traversal_mode == UInt32(MDTBTraversalModeVehicle)
                    ? (territoryPatrolHandoff
                        ? "A court-set runner is turning the vehicle toward the lane before the line"
                        : (territoryPatrolBrace
                            ? "The runner is locking the vehicle line back down while the post steps up behind it"
                            : (territoryPatrolReform
                            ? "The runner is returning from clear and reforming the vehicle line at the lane mouth"
                            : (territoryPatrolScreen
                            ? "A court-set runner is screening the vehicle right at the lane mouth"
                            : "A court-set runner is reading the vehicle outside the lane mouth"))))
                    : (territoryPatrolHandoff
                        ? "A court-set runner is turning the sidewalk approach into a lane handoff"
                        : (territoryPatrolBrace
                            ? "The runner is locking the line back down while the inner post completes the edge"
                            : (territoryPatrolReform
                            ? "The runner is returning from clear while the lane mouth closes back up"
                            : (territoryPatrolScreen
                            ? "A court-set runner is stepping across the lane mouth to screen the entry"
                            : "A court-set runner is posted outside the lane mouth"))))
            )
        }

        if territoryClaimed && territoryWatch > 0.12 && !incidentActive && !searchActive {
            if territorySkimming {
                return (
                    territoryPatrolBrace ? "Territory hardening" : (territoryEntryClamp ? "Entry clamped" : (territoryPatrolReform ? "Territory reforming" : (territoryPatrolScreen ? "Territory screen" : "Territory line"))),
                    territoryVehicleEntry
                        ? (territoryPatrolBrace
                            ? "Rolling the \(territoryName) line / runner and post are locking the vehicle edge back into clamp"
                            : (territoryEntryClamp
                            ? "Rolling the \(territoryName) line / runner and post are clamping the vehicle from opposite angles"
                            : (territoryPatrolReform
                                ? "Rolling the \(territoryName) line / the runner is reforming the edge while the post eases back toward support"
                                : (territoryPatrolScreen
                            ? "Rolling the \(territoryName) line / the runner is screening the vehicle before handoff"
                            : "Rolling the \(territoryName) line / front hold is clocking the vehicle")
                            )))
                        : (territoryPatrolBrace
                            ? "On foot at the \(territoryName) line / runner and post are locking the edge back into clamp"
                            : (territoryEntryClamp
                            ? "On foot at the \(territoryName) line / runner and post are clamping the entry"
                            : (territoryPatrolReform
                                ? "On foot at the \(territoryName) line / the runner is reforming the edge while the post eases back in"
                                : (territoryPatrolScreen
                            ? "On foot at the \(territoryName) line / the runner is screening the entry"
                            : "On foot at the \(territoryName) line / front hold is reading the lane mouth")
                            )))
                )
            }
            if territoryDeeper {
                return (
                    "Claimed turf",
                    territoryVehicleEntry
                        ? (territoryPatrolClear && territoryInnerWatch
                            ? "Runner cleared the line while the inner post owns the vehicle handoff"
                            : (territoryInnerWatch
                                ? "Vehicle push reached deeper \(territoryName) turf / inner post is taking the handoff"
                                : "Vehicle push reached deeper \(territoryName) turf / deep watch is building inside"))
                        : (territoryPatrolClear && territoryInnerWatch
                            ? "Runner cleared the line while the inner post owns the pocket"
                            : (territoryInnerWatch
                                ? "Deeper push into \(territoryName) turf is waking the inner post"
                                : "Deeper push into \(territoryName) turf is drawing a second set of eyes"))
                )
            }
            return (
                "Claimed block",
                "Front and deep holds are reading the block before the fight opens"
            )
        }

        if civilianSources >= 2 && incidentActive {
            return (
                state.street_incident_level >= 0.72 ? "Crowd peeling back" : "Street crowd moving",
                searchActive
                    ? "Two civilians are reacting while the lookout searches \(searchTimer)s"
                    : "Multiple civilians are clearing the block \(incidentTimer)s"
            )
        }

        if incidentActive && state.street_incident_level >= 0.72 {
            return (
                "Street incident live",
                searchActive
                    ? "Traffic is backing off the lane / lookout search \(searchTimer)s"
                    : "The block is still reacting \(incidentTimer)s after the exchange"
            )
        }

        switch state.witness_state {
        case UInt32(MDTBWitnessStateInvestigate):
            return (
                "Witness investigating",
                searchActive
                    ? "A bystander stepped toward the lane / lookout search \(searchTimer)s"
                    : "A bystander stepped toward the lane to check the noise"
            )
        case UInt32(MDTBWitnessStateFlee):
            return (
                "Witness fleeing",
                searchActive
                    ? "The lane spilled into the street / lookout search \(searchTimer)s"
                    : "Gunfire pushed a bystander out of the pocket"
            )
        case UInt32(MDTBWitnessStateCooldown):
            return (
                searchActive ? "Search active" : "Street cooling",
                searchActive
                    ? "Lookout checking the last seen spot / witness cooling \(witnessTimer)s"
                    : (incidentActive
                        ? "Traffic is still giving the block space \(incidentTimer)s"
                        : "The witness is backing off \(witnessTimer)s")
            )
        default:
            if state.bystander_state == UInt32(MDTBWitnessStateInvestigate) {
                return (
                    "Corner bystander watching",
                    searchActive
                        ? "A second civilian is edging closer while the lookout searches \(searchTimer)s"
                        : "Another civilian is testing the edge of the incident"
                )
            }
            if state.bystander_state == UInt32(MDTBWitnessStateFlee) {
                return (
                    "Crowd spreading",
                    searchActive
                        ? "The second bystander is fleeing while search stays live \(searchTimer)s"
                        : "Another civilian is running clear of the block"
                )
            }
            if state.bystander_state == UInt32(MDTBWitnessStateCooldown) {
                return ("Street cooling", "The second bystander is settling \(bystanderTimer)s")
            }
            if searchActive {
                return ("Search active", "Lookout checking your last seen position \(searchTimer)s")
            }
            if state.territory_phase == UInt32(MDTBTerritoryPhaseBoundary) {
                return (
                    "Territory line",
                    territoryVehicleEntry
                        ? "Vehicle is skimming the \(territoryName) line at the lane mouth"
                        : "The \(territoryName) line starts at the lane mouth"
                )
            }
            if territoryClaimed {
                return (
                    "Claimed block",
                    territoryVehicleEntry
                        ? "Vehicle is still inside \(territoryName) turf / the block is reading the pass"
                        : "Inside \(territoryName) turf / the lookout reads you faster here"
                )
            }
            if incidentActive {
                return ("Street holding", "Traffic is still easing around the lane \(incidentTimer)s")
            }
            if trafficHazard >= 0.55 {
                return ("Traffic close", "Crossing pressure is still high near the block")
            }
            return ("Street calm", "No witness reaction outside the lane")
        }
    }

    private static func weaponSwitchHint(state: MDTBEngineState) -> String {
        var hints: [String] = []

        if state.melee_weapon_owned != 0 && state.equipped_weapon_kind != UInt32(MDTBEquippedWeaponLeadPipe) {
            hints.append("1 lead pipe")
        }
        if state.firearm_owned != 0 && state.equipped_weapon_kind != UInt32(MDTBEquippedWeaponPistol) {
            hints.append("2 pistol")
        }

        guard !hints.isEmpty else {
            return ""
        }

        return " / " + hints.joined(separator: " / ")
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
        case UInt32(MDTBDistrictJeffersonPark):
            return "Jefferson Park"
        case UInt32(MDTBDistrictExpositionPark):
            return "Exposition Park"
        case UInt32(MDTBDistrictLeimertPark):
            return "Leimert Park"
        case UInt32(MDTBDistrictCrenshawCorridor):
            return "Crenshaw Corridor"
        case UInt32(MDTBDistrictHistoricSouthCentral):
            return "Historic South Central"
        case UInt32(MDTBDistrictVermontSquare):
            return "Vermont Square"
        case UInt32(MDTBDistrictFlorenceFirestone):
            return "Florence-Firestone"
        case UInt32(MDTBDistrictKoreatown):
            return "Koreatown"
        case UInt32(MDTBDistrictPicoUnion):
            return "Pico-Union"
        case UInt32(MDTBDistrictUniversityPark):
            return "University Park"
        case UInt32(MDTBDistrictSouthPark):
            return "South Park"
        case UInt32(MDTBDistrictInglewood):
            return "Inglewood"
        case UInt32(MDTBDistrictHuntingtonPark):
            return "Huntington Park"
        case UInt32(MDTBDistrictWatts):
            return "Watts"
        case UInt32(MDTBDistrictWillowbrook):
            return "Willowbrook"
        default:
            return "West Adams"
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
        case UInt32(MDTBWorldChunkKoreatownUniversity):
            return "Koreatown-University tile"
        case UInt32(MDTBWorldChunkUniversityParkDowntown):
            return "University Park-Downtown tile"
        case UInt32(MDTBWorldChunkSouthParkIndustrial):
            return "South Park-Industrial tile"
        case UInt32(MDTBWorldChunkCentralSouth):
            return "Central South tile"
        case UInt32(MDTBWorldChunkExpoCrenshaw):
            return "Expo-Crenshaw tile"
        case UInt32(MDTBWorldChunkLeimertBaldwin):
            return "Leimert-Baldwin tile"
        case UInt32(MDTBWorldChunkFlorenceVermont):
            return "Florence-Vermont tile"
        case UInt32(MDTBWorldChunkWattsWillowbrook):
            return "Watts-Willowbrook tile"
        default:
            return "Mid-city West tile"
        }
    }

    private static func blockKindLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBBlockKindResidential):
            return "residential"
        case UInt32(MDTBBlockKindMixedUse):
            return "mixed-use"
        default:
            return "civic hub"
        }
    }

    private static func roadClassLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBRoadClassAvenue):
            return "avenue"
        case UInt32(MDTBRoadClassBoulevard):
            return "boulevard"
        case UInt32(MDTBRoadClassConnector):
            return "connector"
        default:
            return "residential street"
        }
    }

    private static func corridorLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBCorridorFairfaxAve):
            return "Fairfax Ave"
        case UInt32(MDTBCorridorLaBreaAve):
            return "La Brea Ave"
        case UInt32(MDTBCorridorArlingtonAve):
            return "Arlington Ave"
        case UInt32(MDTBCorridorWesternAve):
            return "Western Ave"
        case UInt32(MDTBCorridorVermontAve):
            return "Vermont Ave"
        case UInt32(MDTBCorridorFigueroaSt):
            return "Figueroa St"
        case UInt32(MDTBCorridorCentralAve):
            return "Central Ave"
        case UInt32(MDTBCorridorPicoBlvd):
            return "Pico Blvd"
        case UInt32(MDTBCorridorWashingtonBlvd):
            return "Washington Blvd"
        case UInt32(MDTBCorridorAdamsBlvd):
            return "Adams Blvd"
        case UInt32(MDTBCorridorJeffersonBlvd):
            return "Jefferson Blvd"
        case UInt32(MDTBCorridorExpositionBlvd):
            return "Exposition Blvd"
        case UInt32(MDTBCorridorMartinLutherKingBlvd):
            return "Martin Luther King Jr Blvd"
        case UInt32(MDTBCorridorSlausonAve):
            return "Slauson Ave"
        case UInt32(MDTBCorridorFlorenceAve):
            return "Florence Ave"
        default:
            return "Crenshaw Blvd"
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
        case UInt32(MDTBVehicleKindBicycle):
            return "bicycle"
        case UInt32(MDTBVehicleKindCoupe):
            return "coupe"
        case UInt32(MDTBVehicleKindMoped):
            return "moped"
        case UInt32(MDTBVehicleKindMotorcycle):
            return "motorcycle"
        case UInt32(MDTBVehicleKindSedan):
            return "sedan"
        default:
            return "vehicle"
        }
    }

    private static func vehicleActionLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBVehicleKindBicycle):
            return "pedal"
        case UInt32(MDTBVehicleKindMoped), UInt32(MDTBVehicleKindMotorcycle):
            return "ride"
        default:
            return "drive"
        }
    }

    private static func vehicleExitLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBVehicleKindBicycle), UInt32(MDTBVehicleKindMoped), UInt32(MDTBVehicleKindMotorcycle):
            return "dismount"
        default:
            return "exit"
        }
    }

    private static func attackPhaseLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBMeleeAttackWindup):
            return "windup"
        case UInt32(MDTBMeleeAttackStrike):
            return "strike"
        case UInt32(MDTBMeleeAttackRecovery):
            return "recover"
        default:
            return "idle"
        }
    }

    private static func equippedWeaponLabel(_ value: UInt32) -> String {
        switch value {
        case UInt32(MDTBEquippedWeaponLeadPipe):
            return "lead pipe"
        case UInt32(MDTBEquippedWeaponPistol):
            return "pistol"
        default:
            return "unarmed"
        }
    }

    private static func vehicleSignalColor(for kind: UInt32) -> SIMD4<Float> {
        switch kind {
        case UInt32(MDTBVehicleKindBicycle):
            return SIMD4<Float>(0.36, 0.86, 0.52, 1.0)
        case UInt32(MDTBVehicleKindCoupe):
            return SIMD4<Float>(0.40, 0.74, 0.98, 1.0)
        case UInt32(MDTBVehicleKindMoped):
            return SIMD4<Float>(0.96, 0.78, 0.28, 1.0)
        case UInt32(MDTBVehicleKindMotorcycle):
            return SIMD4<Float>(0.98, 0.42, 0.28, 1.0)
        default:
            return SIMD4<Float>(0.72, 0.84, 0.96, 1.0)
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
