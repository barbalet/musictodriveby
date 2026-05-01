import AppKit
import Foundation
import simd

struct ArtMeshBounds {
    let minimum: SIMD3<Float>
    let maximum: SIMD3<Float>

    var size: SIMD3<Float> {
        maximum - minimum
    }
}

private struct ArtMeshTemplateVertex {
    let position: SIMD3<Float>
    let uv: SIMD2<Float>
    let color: SIMD4<Float>
}

private struct ArtMeshTexture {
    let width: Int
    let height: Int
    let pixels: [UInt8]

    func sample(uv: SIMD2<Float>) -> SIMD4<Float> {
        guard width > 0, height > 0, !pixels.isEmpty else {
            return SIMD4<Float>(repeating: 1.0)
        }

        let wrappedU = uv.x - floor(uv.x)
        let wrappedV = uv.y - floor(uv.y)
        let x = min(max(Int((wrappedU * Float(max(width - 1, 1))).rounded()), 0), width - 1)
        let y = min(max(Int(((1.0 - wrappedV) * Float(max(height - 1, 1))).rounded()), 0), height - 1)
        let offset = ((y * width) + x) * 4
        guard offset + 3 < pixels.count else {
            return SIMD4<Float>(repeating: 1.0)
        }

        return SIMD4<Float>(
            Float(pixels[offset]) / 255.0,
            Float(pixels[offset + 1]) / 255.0,
            Float(pixels[offset + 2]) / 255.0,
            Float(pixels[offset + 3]) / 255.0
        )
    }

    static func load(url: URL) -> ArtMeshTexture? {
        guard
            let image = NSImage(contentsOf: url),
            let cgImage = image.cgImage(forProposedRect: nil, context: nil, hints: nil)
        else {
            return nil
        }

        let width = cgImage.width
        let height = cgImage.height
        let bytesPerRow = width * 4
        var pixels = [UInt8](repeating: 0, count: bytesPerRow * height)

        guard
            let colorSpace = CGColorSpace(name: CGColorSpace.sRGB),
            let context = CGContext(
                data: &pixels,
                width: width,
                height: height,
                bitsPerComponent: 8,
                bytesPerRow: bytesPerRow,
                space: colorSpace,
                bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue
            )
        else {
            return nil
        }

        context.draw(cgImage, in: CGRect(x: 0, y: 0, width: width, height: height))
        return ArtMeshTexture(width: width, height: height, pixels: pixels)
    }
}

private struct ArtMeshAsset {
    let vertices: [ArtMeshTemplateVertex]
    let texture: ArtMeshTexture?
    let bounds: ArtMeshBounds
    let yawOffset: Float
}

enum ArtMeshID: CaseIterable {
    case characterA
    case characterG
    case characterL
    case characterR
    case sedan
    case hatchback
    case raceCar
    case kart
    case blasterA
    case pistol
    case lightSquare
    case signHighway
    case constructionCone
    case commercialLowA
    case commercialWideA
    case commercialTowerA
    case commercialAwning
    case suburbanHouseA
    case suburbanHouseL
    case suburbanFence
    case suburbanPlanter
    case suburbanTree
    case industrialBuildingD
    case industrialBuildingN
    case industrialTank
    case industrialChimney

    fileprivate var folder: String {
        switch self {
        case .characterA, .characterG, .characterL, .characterR:
            return "characters"
        case .sedan, .hatchback, .raceCar, .kart:
            return "vehicles"
        case .blasterA, .pistol:
            return "weapons"
        case .lightSquare, .signHighway, .constructionCone:
            return "roads"
        case .commercialLowA, .commercialWideA, .commercialTowerA, .commercialAwning:
            return "commercial"
        case .suburbanHouseA, .suburbanHouseL, .suburbanFence, .suburbanPlanter, .suburbanTree:
            return "suburban"
        case .industrialBuildingD, .industrialBuildingN, .industrialTank, .industrialChimney:
            return "industrial"
        }
    }

    fileprivate var baseName: String {
        switch self {
        case .characterA:
            return "character-a"
        case .characterG:
            return "character-g"
        case .characterL:
            return "character-l"
        case .characterR:
            return "character-r"
        case .sedan:
            return "sedan"
        case .hatchback:
            return "hatchback-sports"
        case .raceCar:
            return "race"
        case .kart:
            return "kart-oopi"
        case .blasterA:
            return "blaster-a"
        case .pistol:
            return "pistol"
        case .lightSquare:
            return "light-square"
        case .signHighway:
            return "sign-highway"
        case .constructionCone:
            return "construction-cone"
        case .commercialLowA:
            return "low-detail-building-a"
        case .commercialWideA:
            return "low-detail-building-wide-a"
        case .commercialTowerA:
            return "building-skyscraper-a"
        case .commercialAwning:
            return "detail-awning"
        case .suburbanHouseA:
            return "building-type-a"
        case .suburbanHouseL:
            return "building-type-l"
        case .suburbanFence:
            return "fence"
        case .suburbanPlanter:
            return "planter"
        case .suburbanTree:
            return "tree-small"
        case .industrialBuildingD:
            return "building-d"
        case .industrialBuildingN:
            return "building-n"
        case .industrialTank:
            return "detail-tank"
        case .industrialChimney:
            return "chimney-medium"
        }
    }

    fileprivate var yawOffset: Float {
        switch self {
        case .lightSquare, .signHighway, .constructionCone,
             .commercialLowA, .commercialWideA, .commercialTowerA, .commercialAwning,
             .suburbanHouseA, .suburbanHouseL, .suburbanFence, .suburbanPlanter, .suburbanTree,
             .industrialBuildingD, .industrialBuildingN, .industrialTank, .industrialChimney:
            return 0.0
        default:
            return .pi
        }
    }
}

final class ArtMeshCatalog: @unchecked Sendable {
    static let shared = ArtMeshCatalog()

    private let assets: [ArtMeshID: ArtMeshAsset]
    private let lightDirection = simd_normalize(SIMD3<Float>(0.36, 0.82, 0.44))

    private init() {
        var loadedAssets: [ArtMeshID: ArtMeshAsset] = [:]

        for meshID in ArtMeshID.allCases {
            if let asset = Self.loadAsset(for: meshID) {
                loadedAssets[meshID] = asset
            }
        }

        self.assets = loadedAssets
    }

    func hasAsset(_ meshID: ArtMeshID) -> Bool {
        assets[meshID] != nil
    }

    func bounds(for meshID: ArtMeshID) -> ArtMeshBounds? {
        assets[meshID]?.bounds
    }

    func fittedUniformScale(for meshID: ArtMeshID, targetWidth: Float, targetDepth: Float, targetHeight: Float? = nil) -> SIMD3<Float> {
        guard let asset = assets[meshID] else {
            return SIMD3<Float>(repeating: 1.0)
        }

        let size = asset.bounds.size
        let widthScale = targetWidth / max(size.x, 0.001)
        let depthScale = targetDepth / max(size.z, 0.001)
        var uniformScale = min(widthScale, depthScale)

        if let targetHeight {
            let heightScale = targetHeight / max(size.y, 0.001)
            uniformScale = min(uniformScale, heightScale)
        }

        return SIMD3<Float>(repeating: uniformScale)
    }

    func makeVertices(
        for meshID: ArtMeshID,
        position: SIMD3<Float>,
        yaw: Float,
        pitch: Float = 0.0,
        roll: Float = 0.0,
        scale: SIMD3<Float>,
        tint: SIMD4<Float> = SIMD4<Float>(repeating: 1.0),
        lift: Float = 0.0
    ) -> [Vertex] {
        guard let asset = assets[meshID] else {
            return []
        }

        let rotation = yaw + asset.yawOffset
        var vertices: [Vertex] = []
        vertices.reserveCapacity(asset.vertices.count)

        for triangleStart in stride(from: 0, to: asset.vertices.count, by: 3) {
            guard triangleStart + 2 < asset.vertices.count else {
                break
            }

            let v0 = asset.vertices[triangleStart]
            let v1 = asset.vertices[triangleStart + 1]
            let v2 = asset.vertices[triangleStart + 2]

            let p0 = Self.transform(vertex: v0.position, scale: scale, yaw: rotation, pitch: pitch, roll: roll) + position + SIMD3<Float>(0.0, lift, 0.0)
            let p1 = Self.transform(vertex: v1.position, scale: scale, yaw: rotation, pitch: pitch, roll: roll) + position + SIMD3<Float>(0.0, lift, 0.0)
            let p2 = Self.transform(vertex: v2.position, scale: scale, yaw: rotation, pitch: pitch, roll: roll) + position + SIMD3<Float>(0.0, lift, 0.0)

            let edgeA = p1 - p0
            let edgeB = p2 - p0
            let faceNormal = simd_normalize(simd_cross(edgeA, edgeB))
            let diffuse = max(simd_dot(faceNormal, lightDirection), 0.0)
            let shade = 0.42 + (diffuse * 0.58)

            vertices.append(makeVertex(position: p0, templateVertex: v0, asset: asset, tint: tint, shade: shade))
            vertices.append(makeVertex(position: p1, templateVertex: v1, asset: asset, tint: tint, shade: shade))
            vertices.append(makeVertex(position: p2, templateVertex: v2, asset: asset, tint: tint, shade: shade))
        }

        return vertices
    }

    private func makeVertex(position: SIMD3<Float>, templateVertex: ArtMeshTemplateVertex, asset: ArtMeshAsset, tint: SIMD4<Float>, shade: Float) -> Vertex {
        let sampled = asset.texture?.sample(uv: templateVertex.uv) ?? templateVertex.color
        let tintMix = SIMD3<Float>(
            0.66 + (tint.x * 0.34),
            0.66 + (tint.y * 0.34),
            0.66 + (tint.z * 0.34)
        )
        let shaded = SIMD3<Float>(
            min(sampled.x * tintMix.x * shade, 1.0),
            min(sampled.y * tintMix.y * shade, 1.0),
            min(sampled.z * tintMix.z * shade, 1.0)
        )

        return Vertex(
            position: SIMD4<Float>(position.x, position.y, position.z, 1.0),
            color: SIMD4<Float>(shaded.x, shaded.y, shaded.z, min(sampled.w * tint.w, 1.0))
        )
    }

    private static func transform(vertex: SIMD3<Float>, scale: SIMD3<Float>, yaw: Float, pitch: Float, roll: Float) -> SIMD3<Float> {
        let scaled = SIMD3<Float>(vertex.x * scale.x, vertex.y * scale.y, vertex.z * scale.z)
        let pitchCos = cos(pitch)
        let pitchSin = sin(pitch)
        let pitched = SIMD3<Float>(
            scaled.x,
            (scaled.y * pitchCos) - (scaled.z * pitchSin),
            (scaled.y * pitchSin) + (scaled.z * pitchCos)
        )

        let rollCos = cos(roll)
        let rollSin = sin(roll)
        let rolled = SIMD3<Float>(
            (pitched.x * rollCos) - (pitched.y * rollSin),
            (pitched.x * rollSin) + (pitched.y * rollCos),
            pitched.z
        )

        let yawCos = cos(yaw)
        let yawSin = sin(yaw)
        return SIMD3<Float>(
            (rolled.x * yawCos) - (rolled.z * yawSin),
            rolled.y,
            (rolled.x * yawSin) + (rolled.z * yawCos)
        )
    }

    private static func loadAsset(for meshID: ArtMeshID) -> ArtMeshAsset? {
        guard let resourceRoot = artResourceRoot(for: meshID.folder) else {
            return nil
        }

        let objURL = resourceRoot.appendingPathComponent("\(meshID.baseName).obj")
        let mtlURL = resourceRoot.appendingPathComponent("\(meshID.baseName).mtl")
        let materialInfo = loadMaterialInfo(from: mtlURL, fallbackBaseURL: resourceRoot)
        let texture = materialInfo.textureURL.flatMap(ArtMeshTexture.load(url:))
        let materialColors = texture == nil ? materialInfo.materialColors : [:]

        guard let parsedMesh = parseOBJ(url: objURL, materialColors: materialColors) else {
            return nil
        }

        return ArtMeshAsset(
            vertices: parsedMesh.vertices,
            texture: texture,
            bounds: parsedMesh.bounds,
            yawOffset: meshID.yawOffset
        )
    }

    private static func loadMaterialInfo(from mtlURL: URL, fallbackBaseURL: URL) -> (textureURL: URL?, materialColors: [String: SIMD4<Float>]) {
        guard let contents = try? String(contentsOf: mtlURL) else {
            return (nil, [:])
        }

        var textureURL: URL?
        var materialColors: [String: SIMD4<Float>] = [:]
        var currentMaterial: String?
        var currentAlpha: Float = 1.0

        for rawLine in contents.components(separatedBy: .newlines) {
            let trimmed = rawLine.trimmingCharacters(in: .whitespacesAndNewlines)
            guard !trimmed.isEmpty, !trimmed.hasPrefix("#") else {
                continue
            }

            if trimmed.hasPrefix("newmtl ") {
                currentMaterial = String(trimmed.dropFirst(7)).trimmingCharacters(in: .whitespacesAndNewlines)
                currentAlpha = materialColors[currentMaterial ?? ""]?.w ?? 1.0
                continue
            }

            if trimmed.hasPrefix("Kd ") {
                let values = trimmed
                    .split(separator: " ")
                    .dropFirst()
                    .compactMap { Float($0) }
                guard values.count >= 3, let currentMaterial else {
                    continue
                }

                materialColors[currentMaterial] = SIMD4<Float>(values[0], values[1], values[2], currentAlpha)
                continue
            }

            if trimmed.hasPrefix("d ") {
                let values = trimmed
                    .split(separator: " ")
                    .dropFirst()
                    .compactMap { Float($0) }
                guard let alpha = values.first else {
                    continue
                }

                currentAlpha = alpha
                if let currentMaterial {
                    let existing = materialColors[currentMaterial] ?? SIMD4<Float>(repeating: 1.0)
                    materialColors[currentMaterial] = SIMD4<Float>(existing.x, existing.y, existing.z, alpha)
                }
                continue
            }

            if trimmed.hasPrefix("map_Kd ") {
                let texturePath = String(trimmed.dropFirst(7)).trimmingCharacters(in: .whitespacesAndNewlines)
                let directURL = fallbackBaseURL.appendingPathComponent(texturePath)
                if FileManager.default.fileExists(atPath: directURL.path) {
                    textureURL = directURL
                    continue
                }

                let fileName = URL(fileURLWithPath: texturePath).lastPathComponent
                let siblingURL = fallbackBaseURL.appendingPathComponent(fileName)
                if FileManager.default.fileExists(atPath: siblingURL.path) {
                    textureURL = siblingURL
                }
            }
        }

        return (textureURL, materialColors)
    }

    private static func artResourceRoot(for folder: String) -> URL? {
        #if SWIFT_PACKAGE
        let resourceBundle = Bundle.module
        #else
        let resourceBundle = Bundle.main
        #endif

        guard let bundleRoot = resourceBundle.resourceURL else {
            return nil
        }

        let directRoot = bundleRoot.appendingPathComponent("ArtReset/\(folder)")
        if FileManager.default.fileExists(atPath: directRoot.path) {
            return directRoot
        }

        let copiedRoot = bundleRoot.appendingPathComponent("Resources/ArtReset/\(folder)")
        if FileManager.default.fileExists(atPath: copiedRoot.path) {
            return copiedRoot
        }

        return nil
    }

    private static func parseOBJ(url: URL, materialColors: [String: SIMD4<Float>]) -> (vertices: [ArtMeshTemplateVertex], bounds: ArtMeshBounds)? {
        guard let contents = try? String(contentsOf: url) else {
            return nil
        }

        var positions: [SIMD3<Float>] = []
        var texcoords: [SIMD2<Float>] = []
        var triangles: [ArtMeshTemplateVertex] = []
        var currentMaterialColor = SIMD4<Float>(repeating: 1.0)
        var minimum = SIMD3<Float>(repeating: .greatestFiniteMagnitude)
        var maximum = SIMD3<Float>(repeating: -.greatestFiniteMagnitude)

        for rawLine in contents.components(separatedBy: .newlines) {
            let trimmed = rawLine.trimmingCharacters(in: .whitespacesAndNewlines)

            if trimmed.isEmpty || trimmed.hasPrefix("#") || trimmed.hasPrefix("g ") || trimmed.hasPrefix("mtllib ") || trimmed.hasPrefix("vn ") {
                continue
            }

            if trimmed.hasPrefix("usemtl ") {
                let materialName = String(trimmed.dropFirst(7)).trimmingCharacters(in: .whitespacesAndNewlines)
                currentMaterialColor = materialColors[materialName] ?? SIMD4<Float>(repeating: 1.0)
                continue
            }

            if trimmed.hasPrefix("v ") {
                let values = trimmed
                    .split(separator: " ")
                    .dropFirst()
                    .compactMap { Float($0) }
                guard values.count >= 3 else {
                    continue
                }

                let position = SIMD3<Float>(values[0], values[1], values[2])
                positions.append(position)
                minimum = simd_min(minimum, position)
                maximum = simd_max(maximum, position)
                continue
            }

            if trimmed.hasPrefix("vt ") {
                let values = trimmed
                    .split(separator: " ")
                    .dropFirst()
                    .compactMap { Float($0) }
                guard values.count >= 2 else {
                    continue
                }

                texcoords.append(SIMD2<Float>(values[0], values[1]))
                continue
            }

            if trimmed.hasPrefix("f ") {
                let elements = trimmed.split(separator: " ").dropFirst()
                guard elements.count >= 3 else {
                    continue
                }

                let parsedFace = elements.compactMap { token -> ArtMeshTemplateVertex? in
                    let parts = token.split(separator: "/", omittingEmptySubsequences: false)
                    guard let positionIndex = Int(parts[0]), positionIndex > 0, positionIndex <= positions.count else {
                        return nil
                    }

                    let uvIndex = parts.count > 1 ? Int(parts[1]) : nil
                    let uv = if let uvIndex, uvIndex > 0, uvIndex <= texcoords.count {
                        texcoords[uvIndex - 1]
                    } else {
                        SIMD2<Float>(0.5, 0.5)
                    }

                    return ArtMeshTemplateVertex(
                        position: positions[positionIndex - 1],
                        uv: uv,
                        color: currentMaterialColor
                    )
                }

                guard parsedFace.count >= 3 else {
                    continue
                }

                for index in 1 ..< (parsedFace.count - 1) {
                    triangles.append(parsedFace[0])
                    triangles.append(parsedFace[index])
                    triangles.append(parsedFace[index + 1])
                }
            }
        }

        guard !triangles.isEmpty else {
            return nil
        }

        return (
            vertices: triangles,
            bounds: ArtMeshBounds(minimum: minimum, maximum: maximum)
        )
    }
}
