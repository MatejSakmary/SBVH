#pragma once

#include <daxa/daxa.inl>

#define INDEX_COUNT 18

struct IndexBuffer
{
    daxa_u32 indices[INDEX_COUNT];
};
DAXA_ENABLE_BUFFER_PTR(IndexBuffer)

struct AABBGeometryInfo
{
    daxa_f32vec3 position;
    daxa_f32vec3 scale;
};
DAXA_ENABLE_BUFFER_PTR(AABBGeometryInfo)

struct SceneGeometryVertices
{
    daxa_f32vec3 position;
    daxa_f32vec3 normal;
};
DAXA_ENABLE_BUFFER_PTR(SceneGeometryVertices)

struct SceneGeometryIndices
{
    daxa_u32 index;
};
DAXA_ENABLE_BUFFER_PTR(SceneGeometryIndices)

struct TransformData
{
    daxa_f32mat4x4 m_proj_view;
};
DAXA_ENABLE_BUFFER_PTR(TransformData)

struct AABBDrawPC
{
    daxa_BufferPtr(TransformData) transforms;
    daxa_BufferPtr(AABBGeometryInfo) aabb_transforms;
};

struct DrawScenePC
{
    daxa_BufferPtr(TransformData) transforms;
    daxa_BufferPtr(SceneGeometryVertices) vertices;
    daxa_u32 index_offset;
    daxa_f32mat4x4 m_model;
};