#pragma once

#include <daxa/daxa.inl>

#define INDEX_COUNT 18

DAXA_DECL_BUFFER_STRUCT(
    IndexBuffer,
    {
        daxa_u32 indices[INDEX_COUNT];
    }
)

DAXA_DECL_BUFFER_STRUCT(
    AABBGeometryInfo,
    {
        daxa_f32vec3 position;
        daxa_f32vec3 scale;
    }
)

DAXA_DECL_BUFFER_STRUCT(
    SceneGeometryVertices,
    {
        daxa_f32vec3 position;
        daxa_f32vec3 normal;
    }
)

DAXA_DECL_BUFFER_STRUCT(
    SceneGeometryIndices,
    {
        daxa_u32 index;
    }
)

DAXA_DECL_BUFFER_STRUCT(
    TransformData,
    {
        daxa_f32mat4x4 m_proj_view;
    }
)

struct AABBDrawPC
{
    daxa_RWBuffer(TransformData) transforms;
    daxa_RWBuffer(AABBGeometryInfo) aabb_transforms;
};