#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#include <shared/draw_aabb_shared.inl>

DAXA_USE_PUSH_CONSTANT(DrawScenePC)
RWBuffer(SceneGeometryVertices) scene_vertices = daxa_push_constant.vertices;
RWBuffer(TransformData) camera_transforms = daxa_push_constant.transforms;

#if defined(_VERTEX)
// ===================== VERTEX SHADER ===============================
layout (location = 0) out f32vec3 normal_out;
void main()
{
    f32vec4 pre_trans_pos = f32vec4(scene_vertices[gl_VertexIndex + daxa_push_constant.index_offset].position, 1.0);

    f32mat4x4 m_proj_view_model = camera_transforms.m_proj_view * daxa_push_constant.m_model;
    normal_out = scene_vertices[gl_VertexIndex + daxa_push_constant.index_offset].normal;
    gl_Position = m_proj_view_model * pre_trans_pos;
    // gl_Position = pre_trans_pos;
    // gl_Position = f32vec4(0.0, 0.0, 0.0, 1.0);
}

#elif defined(_FRAGMENT)
// ===================== FRAGMENT SHADER ===============================
layout (location = 0) in f32vec3 normal_in;
layout (location = 0) out f32vec4 out_color;

void main()
{
    out_color = f32vec4(normal_in, 1.0);
}
#endif