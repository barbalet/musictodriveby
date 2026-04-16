import simd

extension simd_float4x4 {
    static func perspectiveProjection(fovY: Float, aspect: Float, nearZ: Float, farZ: Float) -> simd_float4x4 {
        let yScale = 1 / tan(fovY * 0.5)
        let xScale = yScale / aspect
        let zRange = farZ - nearZ
        let zScale = -(farZ + nearZ) / zRange
        let wzScale = -(2 * farZ * nearZ) / zRange

        return simd_float4x4(
            SIMD4<Float>(xScale, 0, 0, 0),
            SIMD4<Float>(0, yScale, 0, 0),
            SIMD4<Float>(0, 0, zScale, -1),
            SIMD4<Float>(0, 0, wzScale, 0)
        )
    }

    static func lookAt(eye: SIMD3<Float>, target: SIMD3<Float>, up: SIMD3<Float>) -> simd_float4x4 {
        let forward = simd_normalize(target - eye)
        let right = simd_normalize(simd_cross(forward, up))
        let cameraUp = simd_cross(right, forward)

        return simd_float4x4(
            SIMD4<Float>(right.x, cameraUp.x, -forward.x, 0),
            SIMD4<Float>(right.y, cameraUp.y, -forward.y, 0),
            SIMD4<Float>(right.z, cameraUp.z, -forward.z, 0),
            SIMD4<Float>(-simd_dot(right, eye), -simd_dot(cameraUp, eye), simd_dot(forward, eye), 1)
        )
    }
}
