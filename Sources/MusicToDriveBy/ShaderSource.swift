enum ShaderSource {
    static let grayboxScene = """
    #include <metal_stdlib>
    using namespace metal;

    struct VertexIn {
        float4 position [[attribute(0)]];
        float4 color [[attribute(1)]];
    };

    struct Uniforms {
        float4x4 viewProjectionMatrix;
    };

    struct VertexOut {
        float4 position [[position]];
        float4 color;
    };

    vertex VertexOut vertex_main(VertexIn in [[stage_in]],
                                 constant Uniforms& uniforms [[buffer(1)]]) {
        VertexOut out;
        out.position = uniforms.viewProjectionMatrix * in.position;
        out.color = in.color;
        return out;
    }

    fragment float4 fragment_main(VertexOut in [[stage_in]]) {
        return in.color;
    }
    """
}
