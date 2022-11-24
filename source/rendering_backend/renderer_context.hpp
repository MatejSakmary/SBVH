#pragma once

#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>

struct RendererContext
{
    struct MainTaskList
    {
        struct TaskListImages
        {
            daxa::TaskImageId t_swapchain_image;
        };

        daxa::TaskList task_list;
        TaskListImages images;
    };

    struct Pipelines
    {
        daxa::RasterPipeline p_draw_AABB;
    };

    daxa::Context vulkan_context;
    daxa::Device device;
    daxa::Swapchain swapchain;
    daxa::PipelineCompiler pipeline_compiler;

    daxa::ImageId swapchain_image;

    MainTaskList main_task_list;
    Pipelines pipelines;
};