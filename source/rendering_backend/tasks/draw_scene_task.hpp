#pragma once

#include <span>

#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>
#include <daxa/utils/math_operators.hpp>

#include "../../types.hpp"
#include "../renderer_context.hpp"
#include "../shared/draw_aabb_shared.inl"

inline static const daxa::RasterPipelineInfo DRAW_SCENE_TASK_RASTER_PIPE_INFO 
{
    .vertex_shader_info = daxa::ShaderInfo{
        .source = daxa::ShaderFile{"scene.glsl"},
        .compile_options = {
            .defines = {{"_VERTEX", ""}},
        },
    },
    .fragment_shader_info = daxa::ShaderInfo{
        .source = daxa::ShaderFile{"scene.glsl"},
        .compile_options = {
            .defines = {{"_FRAGMENT", ""}},
        },
    },
    .color_attachments = {
        daxa::RenderAttachment{
            .format = daxa::Format::B8G8R8A8_SRGB,
        },
    },
    .raster = {
        .primitive_topology = daxa::PrimitiveTopology::TRIANGLE_LIST,
        .primitive_restart_enable = false,
        .polygon_mode = daxa::PolygonMode::FILL
    },
    .push_constant_size = sizeof(DrawScenePC),
};

inline void task_draw_scene(RendererContext & context)
{
    context.main_task_list.task_list.add_task({
        .used_buffers =
        {
            {
                context.main_task_list.buffers.t_scene_vertices,
                daxa::TaskBufferAccess::SHADER_READ_ONLY,
            },
            {
                context.main_task_list.buffers.t_scene_indices,
                daxa::TaskBufferAccess::SHADER_READ_ONLY,
            },
            {
                context.main_task_list.buffers.t_transform_data,
                daxa::TaskBufferAccess::SHADER_READ_ONLY,
            },
        },
        .used_images =
        {
            { 
                context.main_task_list.images.t_swapchain_image,
                daxa::TaskImageAccess::SHADER_WRITE_ONLY,
                daxa::ImageMipArraySlice{} 
            }
        },
        .task = [&](daxa::TaskRuntime const & runtime)
        {
            auto cmd_list = runtime.get_command_list();
            auto dimensions = context.swapchain.get_surface_extent();
            auto swapchain_image = runtime.get_images(context.main_task_list.images.t_swapchain_image);
            auto index_buffer = runtime.get_buffers(context.main_task_list.buffers.t_scene_indices);
            auto vertex_buffer = runtime.get_buffers(context.main_task_list.buffers.t_scene_vertices);
            auto transforms_buffer = runtime.get_buffers(context.main_task_list.buffers.t_transform_data);
            cmd_list.begin_renderpass({
                .color_attachments = 
                {{
                    .image_view = swapchain_image[0].default_view(),
                    .load_op = daxa::AttachmentLoadOp::CLEAR,
                    .clear_value = std::array<f32, 4>{0.02, 0.02, 0.02, 1.0},
                }},
                .depth_attachment = {},
                .render_area = {.x = 0, .y = 0, .width = dimensions.x , .height = dimensions.y}
            });

            cmd_list.set_pipeline(context.pipelines.p_draw_scene);

            // NOTE(msakmary) I can't put const auto & object here since than the span constructor complains
            // and I don't know how to fig this
            for(auto & object : context.render_info.objects)
            {
                for(const auto & mesh : object.meshes)
                {
                    cmd_list.push_constant(DrawScenePC{
                        .transforms = context.device.get_device_address(transforms_buffer[0]),
                        .vertices = context.device.get_device_address(vertex_buffer[0]),
                        .index_offset = mesh.index_offset,
                        .m_model = daxa::math_operators::mat_from_span<daxa::f32, 4, 4>(
                            std::span<daxa::f32, 4 * 4>{glm::value_ptr(object.model_transform), 4 * 4})
                    });
                    cmd_list.set_index_buffer(index_buffer[0], mesh.index_offset, sizeof(u32));
                    cmd_list.draw_indexed({ .index_count = mesh.index_count });
                }
            }
            cmd_list.end_renderpass();
        },
        .debug_name = "draw scene",
    });
}