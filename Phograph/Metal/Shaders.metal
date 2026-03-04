#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
};

// Generates a fullscreen quad from 6 vertices (2 triangles).
// No vertex buffer needed - positions are computed from vertex_id.
vertex VertexOut fullscreenQuadVertex(uint vid [[vertex_id]]) {
    // Two triangles covering the full screen:
    // Triangle 1: (0,1) -> (0,2) -> (1,0)  mapped to NDC
    // Triangle 2: (1,0) -> (0,2) -> (1,2)
    float2 positions[6] = {
        float2(-1, -1), float2(-1,  1), float2( 1, -1),
        float2( 1, -1), float2(-1,  1), float2( 1,  1)
    };
    // Texture coordinates: flip Y so top-left is (0,0)
    float2 texCoords[6] = {
        float2(0, 1), float2(0, 0), float2(1, 1),
        float2(1, 1), float2(0, 0), float2(1, 0)
    };

    VertexOut out;
    out.position = float4(positions[vid], 0, 1);
    out.texCoord = texCoords[vid];
    return out;
}

// Samples the pixel buffer texture with nearest-neighbor filtering.
fragment float4 fullscreenQuadFragment(VertexOut in [[stage_in]],
                                        texture2d<float> tex [[texture(0)]]) {
    constexpr sampler nearestSampler(mag_filter::nearest, min_filter::nearest);
    return tex.sample(nearestSampler, in.texCoord);
}
