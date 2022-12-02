#pragma once

#include <vector>
#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>
#include <daxa/utils/imgui.hpp>

#include "shared/draw_aabb_shared.inl"

struct RendererContext
{
    struct Buffers
    {
        template<typename T>
        struct SharedBuffer
        {
            T cpu_buffer;
            daxa::BufferId gpu_buffer;
        };

        SharedBuffer<TransformData> transforms_buffer;
        SharedBuffer<IndexBuffer> index_buffer;
        SharedBuffer<std::vector<AABBGeometryInfo>> aabb_info_buffer;
        SharedBuffer<std::vector<SceneGeometryVertices>> scene_vertices;
        SharedBuffer<std::vector<SceneGeometryIndices>> scene_indices;
    };

    struct MainTaskList
    {
        struct TaskListImages
        {
            daxa::TaskImageId t_swapchain_image;
            daxa::TaskImageId t_depth_image;
        };

        struct TaskListBuffers
        {
            daxa::TaskBufferId t_cube_indices;
            daxa::TaskBufferId t_transform_data;
            daxa::TaskBufferId t_aabb_infos;

            daxa::TaskBufferId t_scene_vertices;
            daxa::TaskBufferId t_scene_indices;
        };

        daxa::TaskList task_list;
        TaskListImages images;
        TaskListBuffers buffers;
    };

    struct Pipelines
    {
        daxa::RasterPipeline p_draw_AABB;
        daxa::RasterPipeline p_draw_scene;
    };

    struct Conditionals
    {
        b32 fill_indices = true;
        b32 fill_transforms = true;
        b32 fill_aabbs = true;
        b32 fill_scene_geometry = false;
    };

    // TODO(msakmary) perhaps reconsider moving this to Scene?
    struct SceneRenderInfo
    {
        struct RenderMeshInfo
        {
            u32 index_buffer_offset;
            u32 index_offset;
            u32 index_count;
        };
        struct RenderObjectInfo
        {
            f32mat4x4 model_transform;
            std::vector<RenderMeshInfo> meshes;
        };

        std::vector<RenderObjectInfo> objects;
    };

    daxa::Context vulkan_context;
    daxa::Device device;
    daxa::Swapchain swapchain;
    daxa::PipelineCompiler pipeline_compiler;

    daxa::ImageId swapchain_image;
    daxa::ImageId depth_image;
    daxa::ImGuiRenderer imgui_renderer;

    Buffers buffers;
    MainTaskList main_task_list;
    Pipelines pipelines;

    Conditionals conditionals;
    SceneRenderInfo render_info;
};