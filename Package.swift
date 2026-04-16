// swift-tools-version: 6.2
import PackageDescription

let package = Package(
    name: "MusicToDriveBy",
    platforms: [
        .macOS(.v14),
    ],
    products: [
        .executable(
            name: "MusicToDriveBy",
            targets: ["MusicToDriveBy"]
        ),
    ],
    targets: [
        .executableTarget(
            name: "MusicToDriveBy",
            dependencies: ["EngineCore"],
            path: "Sources/MusicToDriveBy",
            linkerSettings: [
                .linkedFramework("AppKit"),
                .linkedFramework("Metal"),
                .linkedFramework("MetalKit"),
                .linkedFramework("QuartzCore"),
                .linkedFramework("SwiftUI"),
            ]
        ),
        .target(
            name: "EngineCore",
            path: "Sources/EngineCore",
            publicHeadersPath: "include"
        ),
    ]
)
