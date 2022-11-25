#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#include <shared/draw_aabb_shared.inl>

DAXA_USE_PUSH_CONSTANT(AABBDrawPC)
RWBuffer(TransformData) transforms = daxa_push_constant.transforms;

#if defined(_VERTEX)
// ===================== VERTEX SHADER ===============================
f32vec3 vertices[8] = f32vec3[](
    f32vec3( 0.0,  1.0, 0.0 ),
    f32vec3( 0.0,  0.0, 0.0 ),
    f32vec3( 1.0,  0.0, 0.0 ),
    f32vec3( 1.0,  1.0, 0.0 ),

    f32vec3( 1.0,  1.0, 1.0 ),
    f32vec3( 0.0,  1.0, 1.0 ),
    f32vec3( 0.0,  0.0, 1.0 ),
    f32vec3( 1.0,  0.0, 1.0 )
);

void main()
{
    vec4 pre_trans_pos = vec4(vertices[gl_VertexIndex], 1.0);
    mat4 m_proj_view_model = transforms.m_proj_view * daxa_push_constant.m_model;
    gl_Position = m_proj_view_model * pre_trans_pos;
}

#elif defined(_FRAGMENT)
// ===================== FRAGMENT SHADER ===============================
layout (location = 0) out f32vec4 out_color;

void main()
{
    out_color = f32vec4(1.0, 0.0, 0.0, 1.0);
}
#endif