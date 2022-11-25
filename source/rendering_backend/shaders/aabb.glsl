#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#include <shared/draw_aabb_shared.inl>

DAXA_USE_PUSH_CONSTANT(AABBDrawPC)
#if defined(_VERTEX)
// ===================== VERTEX SHADER ===============================
layout (location = 0) out f32vec2 out_uv;

f32vec2 vertices[4] = f32vec2[](
    f32vec2( 0.9, -0.9 ),
    f32vec2( 0.9,  0.9 ),
    f32vec2(-0.9,  0.9 ),
    f32vec2(-0.9, -0.9 )
);

u32 indices[6] = u32[](1,2,3,1,0,0);

void main()
{
    out_uv = f32vec2((vertices[indices[gl_VertexIndex]] + 1.0) / 2.0);
    gl_Position = vec4(vertices[indices[gl_VertexIndex]], 0.0, 1.0);
    // out_uv = f32vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    // gl_Position = vec4(out_uv * 2.0 - 1.0, 0.0, 1.0);
}

#elif defined(_FRAGMENT)
// ===================== FRAGMENT SHADER ===============================
layout (location = 0) in f32vec2 in_uv;
layout (location = 0) out f32vec4 out_color;

void main()
{
    out_color = f32vec4(in_uv, 0.0, 1.0);
}
#endif