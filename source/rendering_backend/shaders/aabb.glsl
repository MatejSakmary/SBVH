#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#include <shared/draw_aabb_shared.inl>

DAXA_USE_PUSH_CONSTANT(AABBDrawPC)
RWBuffer(TransformData) transforms = daxa_push_constant.transforms;

#if defined(_VERTEX)
// ===================== VERTEX SHADER ===============================
f32vec3 vertices[8] = f32vec3[](
    f32vec3( 0.0,  0.5, 0.0 ),
    f32vec3( 0.0,  0.0, 0.0 ),
    f32vec3( 0.5,  0.0, 0.0 ),
    f32vec3( 0.5,  0.5, 0.0 ),

    f32vec3( 0.6,  0.6, 1.0 ),
    f32vec3( 0.1,  0.6, 1.0 ),
    f32vec3( 0.1,  0.1, 1.0 ),
    f32vec3( 0.6,  0.1, 1.0 )
);

void main()
{
    gl_Position = vec4(vertices[gl_VertexIndex], 1.0);
}

#elif defined(_FRAGMENT)
// ===================== FRAGMENT SHADER ===============================
layout (location = 0) out f32vec4 out_color;

void main()
{
    out_color = f32vec4(1.0, 0.0, 0.0, 1.0);
}
#endif