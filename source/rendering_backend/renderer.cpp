#include <dlfcn.h>
#include "renderer.hpp"

Renderer::Renderer(const AppWindow & window) :
    camera {{.position = {0.0, 0.0, 0.0}, .front = {0.0, 0.0, 1.0}, .up = {0.0, 1.0, 0.0}}},
    context {.vulkan_context = daxa::create_context({.enable_validation = true}),
            .device = context.vulkan_context.create_device({.debug_name = "Daxa device"})}
{
    context.swapchain = context.device.create_swapchain({ 
        .native_window = window.get_native_handle(),
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
    context.pipelines.p_draw_scene = context.pipeline_compiler.create_raster_pipeline(DRAW_SCENE_TASK_RASTER_PIPE_INFO).value();

    context.buffers.transforms_buffer.gpu_buffer = context.device.create_buffer({
        .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
        .size = sizeof(TransformData),
        .debug_name = "transform info"
    });

    // ================  TODO(msakmary) move this where it belongs later ========================
    context.buffers.aabb_info_buffer.cpu_buffer.reserve(10 * 10 * 10);
    for(size_t x = 0; x < 10; x++)
    {
        for(size_t y = 0; y < 10; y++)
        {
            for(size_t z = 0; z < 10; z++)
            {
                context.buffers.aabb_info_buffer.cpu_buffer.emplace_back(AABBGeometryInfo {
                    .position = {f32(x) * 2.0f, f32(y) * 2.0f, f32(z) * 2.0f},
                    .scale = {1.0f - f32(x) * 0.01f, 1.0f - f32(y) * 0.01f, 1.0f - f32(z) * 0.01f}
                });
            }
        }
    }
    context.buffers.aabb_info_buffer.gpu_buffer = context.device.create_buffer({
        .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
        .size = static_cast<u32>(sizeof(AABBGeometryInfo) * context.buffers.aabb_info_buffer.cpu_buffer.size()),
        .debug_name = "aabb_infos"
    });
    // ==========================================================================================

    context.buffers.index_buffer.gpu_buffer = context.device.create_buffer({
        .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
        .size = sizeof(IndexBuffer),
        .debug_name = "cube_indices"
    });

    context.buffers.index_buffer.cpu_buffer = 
    {
        0, 1, 2, 3, 4, 5, 0, 3, 0xFFFFFFFF,
        6, 5, 4, 7, 2, 1, 6, 7, 0xFFFFFFFF
    };
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(window.get_glfw_window_handle(), true);
    auto &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    context.imgui_renderer = daxa::ImGuiRenderer({
        .device = context.device,
        .pipeline_compiler = context.pipeline_compiler,
        .format = context.swapchain.get_format(),
    });
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

    context.main_task_list.buffers.t_aabb_infos = 
        context.main_task_list.task_list.create_task_buffer(
        {
            .initial_access = daxa::AccessConsts::NONE,
            .debug_name = "t_aabb_infos"
        }
    );
    context.main_task_list.task_list.add_runtime_buffer(
        context.main_task_list.buffers.t_aabb_infos,
        context.buffers.aabb_info_buffer.gpu_buffer);


    context.main_task_list.buffers.t_transform_data = 
        context.main_task_list.task_list.create_task_buffer(
        {
            .initial_access = daxa::AccessConsts::NONE,
            .debug_name = "t_transform_data"
        }
    );
    context.main_task_list.task_list.add_runtime_buffer(
        context.main_task_list.buffers.t_transform_data,
        context.buffers.transforms_buffer.gpu_buffer);

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

    context.main_task_list.buffers.t_scene_vertices = 
        context.main_task_list.task_list.create_task_buffer(
        {
            .initial_access = daxa::AccessConsts::NONE,
            .debug_name = "t_scene_vertices"
        }
    );
    
    context.main_task_list.buffers.t_scene_indices = 
        context.main_task_list.task_list.create_task_buffer(
        {
            .initial_access = daxa::AccessConsts::NONE,
            .debug_name = "t_scene_indices"
        }
    );

    task_fill_buffers(context);
    task_draw_scene(context);
    task_draw_AABB(context);
    task_draw_imgui(context);
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
            DEBUG_OUT(new_pipeline.to_string());
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
            DEBUG_OUT(new_pipeline.to_string());
            if (new_pipeline.is_ok())
            {
                pipeline = new_pipeline.value();
                return true;
            }
        }
        return false;
    };

    // ==============  TODO(msakmary) move this somewhere where it belongs ==================
    f32 aspect = f32(context.swapchain.get_surface_extent().x) / f32(context.swapchain.get_surface_extent().y);
    f32mat4x4 m_proj = glm::perspective(glm::radians(50.0f), aspect, 0.1f, 500.0f);
    /* GLM is using OpenGL standard where Y coordinate of the clip coordinates is inverted */
    m_proj[1][1] *= -1;
    auto m_view = camera.get_view_matrix();
    f32mat4x4 m_proj_view = m_proj * camera.get_view_matrix();
    context.buffers.transforms_buffer.cpu_buffer =
    {
        .m_proj_view = *reinterpret_cast<daxa::f32mat4x4 *>(&m_proj_view)
    };
    // ======================================================================================

    context.conditionals.fill_transforms = true;

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
    ui_update();
    context.main_task_list.task_list.execute();
    reload_raster_pipeline(context.pipelines.p_draw_AABB);
    reload_raster_pipeline(context.pipelines.p_draw_scene);
}

void Renderer::reload_scene_data(const Scene & scene)
{
    if(!context.buffers.scene_vertices.gpu_buffer.is_empty())
    {
        context.main_task_list.task_list.remove_runtime_buffer(
            context.main_task_list.buffers.t_scene_vertices,
            context.buffers.scene_vertices.gpu_buffer);

        context.device.destroy_buffer(context.buffers.scene_vertices.gpu_buffer);
        context.buffers.scene_vertices.cpu_buffer.clear();
    }
    if(!context.buffers.scene_indices.gpu_buffer.is_empty())
    {
        context.main_task_list.task_list.remove_runtime_buffer(
            context.main_task_list.buffers.t_scene_indices,
            context.buffers.scene_indices.gpu_buffer);

        context.device.destroy_buffer(context.buffers.scene_indices.gpu_buffer);
        context.buffers.scene_indices.cpu_buffer.clear();
    }
    context.render_info.objects.clear();
    size_t scene_vertex_cnt = 0;
    size_t scene_index_cnt = 0;
    // pack scene vertices and scene indices into their separate GPU buffers
    for(auto scene_runtime_object : scene.runtime_scene_objects)
    {
        context.render_info.objects.push_back({
            .model_transform = scene_runtime_object.transform,
        });
        auto & object = context.render_info.objects.back();

        for(auto scene_runtime_mesh : scene_runtime_object.meshes)
        {
            
            object.meshes.push_back({
                .index_buffer_offset = static_cast<u32>(scene_index_cnt),
                .index_offset = static_cast<u32>(scene_vertex_cnt),
                .index_count = static_cast<u32>(scene_runtime_mesh.indices.size()),
            });
            auto & mesh = object.meshes.back();

            context.buffers.scene_vertices.cpu_buffer.resize(scene_vertex_cnt + scene_runtime_mesh.vertices.size());
            memcpy(context.buffers.scene_vertices.cpu_buffer.data() + scene_vertex_cnt,
                   scene_runtime_mesh.vertices.data(),
                   sizeof(SceneGeometryVertices) * scene_runtime_mesh.vertices.size());
            scene_vertex_cnt += scene_runtime_mesh.vertices.size();

            context.buffers.scene_indices.cpu_buffer.resize(scene_index_cnt + scene_runtime_mesh.indices.size());
            memcpy(context.buffers.scene_indices.cpu_buffer.data() + scene_index_cnt,
                   scene_runtime_mesh.indices.data(),
                   sizeof(u32) * scene_runtime_mesh.indices.size());
            scene_index_cnt += scene_runtime_mesh.indices.size();
        }
    }

    context.buffers.scene_vertices.gpu_buffer = context.device.create_buffer({
        .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
        .size = static_cast<u32>(scene_vertex_cnt * sizeof(SceneGeometryVertices)),
        .debug_name = "scene_geometry_vertices"
    });

    context.main_task_list.task_list.add_runtime_buffer(
        context.main_task_list.buffers.t_scene_vertices,
        context.buffers.scene_vertices.gpu_buffer);

    context.buffers.scene_indices.gpu_buffer = context.device.create_buffer({
        .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
        .size = static_cast<u32>(scene_index_cnt * sizeof(SceneGeometryIndices)),
        .debug_name = "scene_geometry_indices"
    });

    context.main_task_list.task_list.add_runtime_buffer(
        context.main_task_list.buffers.t_scene_indices,
        context.buffers.scene_indices.gpu_buffer);

    context.conditionals.fill_scene_geometry = true;
    DEBUG_OUT("[Renderer::reload_scene_data()] scene reload successfull");
}

Renderer::~Renderer()
{
    context.device.wait_idle();
    ImGui_ImplGlfw_Shutdown();
    context.device.destroy_buffer(context.buffers.index_buffer.gpu_buffer);
    context.device.destroy_buffer(context.buffers.transforms_buffer.gpu_buffer);
    context.device.destroy_buffer(context.buffers.aabb_info_buffer.gpu_buffer);
    if(!context.buffers.scene_vertices.gpu_buffer.is_empty())
    {
        context.device.destroy_buffer(context.buffers.scene_vertices.gpu_buffer);
    }
    if(!context.buffers.scene_indices.gpu_buffer.is_empty())
    {
        context.device.destroy_buffer(context.buffers.scene_indices.gpu_buffer);
    }
    context.device.collect_garbage();
}