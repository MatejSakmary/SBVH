#pragma once

#include <vector>
#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>

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
    };

    struct MainTaskList
    {
        struct TaskListImages
        {
            daxa::TaskImageId t_swapchain_image;
        };

        struct TaskListBuffers
        {
            daxa::TaskBufferId t_cube_indices;
            daxa::TaskBufferId t_transform_data;
            daxa::TaskBufferId t_aabb_infos;
        };

        daxa::TaskList task_list;
        TaskListImages images;
        TaskListBuffers buffers;
    };

    struct Pipelines
    {
        daxa::RasterPipeline p_draw_AABB;
    };

    struct Conditionals
    {
        b32 fill_indices = true;
        b32 fill_transforms = true;
        b32 fill_aabbs = true;
    };

    daxa::Context vulkan_context;
    daxa::Device device;
    daxa::Swapchain swapchain;
    daxa::PipelineCompiler pipeline_compiler;

    daxa::ImageId swapchain_image;

    Buffers buffers;
    MainTaskList main_task_list;
    Pipelines pipelines;

    Conditionals conditionals;
};