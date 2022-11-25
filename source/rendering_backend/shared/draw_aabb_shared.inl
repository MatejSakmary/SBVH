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
    TransformData,
    {
        daxa_f32mat4x4 m_proj_view;
    }
)

struct AABBDrawPC
{
    daxa_RWBuffer(TransformData) transforms;
    daxa_f32mat4x4 m_model;
};