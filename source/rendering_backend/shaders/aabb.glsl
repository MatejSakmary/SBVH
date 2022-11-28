#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#include <shared/draw_aabb_shared.inl>

DAXA_USE_PUSH_CONSTANT(AABBDrawPC)
RWBuffer(TransformData) camera_transforms = daxa_push_constant.transforms;
RWBuffer(AABBGeometryInfo) aabb_transforms = daxa_push_constant.aabb_transforms;

#if defined(_VERTEX)
layout (location = 0) out f32vec4 aabb_color;
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
    f32vec4 pre_trans_pos = f32vec4(vertices[gl_VertexIndex], 1.0);
    f32vec3 pos = aabb_transforms[gl_InstanceIndex].position;
    f32vec3 scale = aabb_transforms[gl_InstanceIndex].scale;
    f32mat4x4 m_model = f32mat4x4(
        f32vec4( scale.x,  0.0,     0.0,    0.0), // first column
        f32vec4(   0.0,   scale.y,  0.0,    0.0), // second column
        f32vec4(   0.0,     0.0,   scale.z, 0.0), // third column
        f32vec4(  pos.x,   pos.y,   pos.z,  1.0)  // fourth column
    );
    mat4 m_proj_view_model = camera_transforms.m_proj_view * m_model;
    aabb_color = f32vec4(pos / 20, 1.0);
    gl_Position = m_proj_view_model * pre_trans_pos;
}

#elif defined(_FRAGMENT)
// ===================== FRAGMENT SHADER ===============================
layout (location = 0) in f32vec4 aabb_color;
layout (location = 0) out f32vec4 out_color;

void main()
{
    out_color = aabb_color;
}
#endif