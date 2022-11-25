#include "renderer.hpp"

Renderer::Renderer(daxa::NativeWindowHandle window_handle) :
    context{.vulkan_context = daxa::create_context({.enable_validation = true}),
            .device = context.vulkan_context.create_device({.debug_name = "Daxa device"})}
{
    context.swapchain = context.device.create_swapchain({ 
        .native_window = window_handle,
        .native_window_platform = daxa::NativeWindowPlatform::XLIB_API,
        .present_mode = daxa::PresentMode::DOUBLE_BUFFER_WAIT_FOR_VBLANK,
        .image_usage = daxa::ImageUsageFlagBits::TRANSFER_DST | daxa::ImageUsageFlagBits::COLOR_ATTACHMENT,
        .debug_name = "Swapchain",
    });

    context.pipeline_compiler = context.device.create_pipeline_compiler({
        .shader_compile_options = {
            .root_paths = { DAXA_SHADER_INCLUDE_DIR, "source/rendering_backend", "source/rendering_backend/shaders",},
            .language = daxa::ShaderLanguage::GLSL,
        },
        .debug_name = "Pipeline Compiler",
    });

    context.pipelines.p_draw_AABB = context.pipeline_compiler.create_raster_pipeline(DRAW_AABB_TASK_RASTER_PIPE_INFO).value();

    context.buffers.transforms_buffer.gpu_buffer = context.device.create_buffer({
        .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
        .size = sizeof(TransformData),
        .debug_name = "transform info"
    });

    context.buffers.index_buffer.gpu_buffer = context.device.create_buffer({
        .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
        .size = sizeof(IndexBuffer),
        .debug_name = "cube_indices"
    });

    context.buffers.index_buffer.cpu_buffer = {
        0, 1, 2, 3, 4, 5, 0, 3, 0xFFFFFFFF,
        6, 5, 4, 7, 2, 1, 6, 7, 0xFFFFFFFF};
    create_main_task();
}

void Renderer::create_main_task()
{
    context.main_task_list.task_list = daxa::TaskList({
        .device = context.device,
        .dont_reorder_tasks = false,
        .dont_use_split_barriers = false,
        .swapchain = context.swapchain,
        .debug_name = "main_tasklist"
    });

    context.main_task_list.images.t_swapchain_image = 
        context.main_task_list.task_list.create_task_image(
        {
            .initial_access = daxa::AccessConsts::NONE,
            .initial_layout = daxa::ImageLayout::UNDEFINED,
            .swapchain_image = true,
            .debug_name = "t_swapchain_image"
        }
    );

    context.main_task_list.buffers.t_cube_indices = 
        context.main_task_list.task_list.create_task_buffer(
        {
            .initial_access = daxa::AccessConsts::NONE,
            .debug_name = "t_cube_indices"
        }
    );

    context.main_task_list.task_list.add_runtime_buffer(
        context.main_task_list.buffers.t_cube_indices,
        context.buffers.index_buffer.gpu_buffer);

    task_fill_index_buffer(context);
    task_draw_AABB(context);
    context.main_task_list.task_list.submit({});
    context.main_task_list.task_list.present({});
    context.main_task_list.task_list.complete();
}

void Renderer::resize()
{
    context.swapchain.resize();
}

void Renderer::draw()
{
    auto reload_raster_pipeline = [this](daxa::RasterPipeline & pipeline) -> bool
    {
        if (context.pipeline_compiler.check_if_sources_changed(pipeline))
        {
            auto new_pipeline = context.pipeline_compiler.recreate_raster_pipeline(pipeline);
            std::cout << new_pipeline.to_string() << std::endl;
            if (new_pipeline.is_ok())
            {
                pipeline = new_pipeline.value();
                return true;
            }
        }
        return false;
    };
    auto reload_compute_pipeline = [this](daxa::ComputePipeline & pipeline) -> bool
    {
        if (context.pipeline_compiler.check_if_sources_changed(pipeline))
        {
            auto new_pipeline = context.pipeline_compiler.recreate_compute_pipeline(pipeline);
            std::cout << new_pipeline.to_string() << std::endl;
            if (new_pipeline.is_ok())
            {
                pipeline = new_pipeline.value();
                return true;
            }
        }
        return false;
    };

    context.main_task_list.task_list.remove_runtime_image(
        context.main_task_list.images.t_swapchain_image,
        context.swapchain_image);

    context.swapchain_image = context.swapchain.acquire_next_image();

    context.main_task_list.task_list.add_runtime_image(
        context.main_task_list.images.t_swapchain_image,
        context.swapchain_image);

    if(context.swapchain_image.is_empty()) 
    {
        DEBUG_OUT("[Renderer::draw()] Got empty image from swapchain");
        return;
    }
    context.main_task_list.task_list.execute();
    reload_raster_pipeline(context.pipelines.p_draw_AABB);
}

Renderer::~Renderer()
{
    context.device.wait_idle();
    context.device.destroy_buffer(context.buffers.index_buffer.gpu_buffer);
    context.device.destroy_buffer(context.buffers.transforms_buffer.gpu_buffer);
    context.device.collect_garbage();
}