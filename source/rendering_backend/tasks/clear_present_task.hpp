#pragma once

#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>

#include "../../types.hpp"
#include "../renderer_context.hpp"
#include "../shared/draw_aabb_shared.inl"

inline static const daxa::RasterPipelineInfo DRAW_AABB_TASK_RASTER_PIPE_INFO 
{
    .vertex_shader_info = daxa::ShaderInfo{
        .source = daxa::ShaderFile{"aabb.glsl"},
        .compile_options = {
            .defines = {{"_VERTEX", ""}},
        },
    },
    .fragment_shader_info = daxa::ShaderInfo{
        .source = daxa::ShaderFile{"aabb.glsl"},
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
        .polygon_mode = daxa::PolygonMode::LINE
    },
    .push_constant_size = sizeof(AABBDrawPC),
};

inline void task_draw_AABB(RendererContext & context)
{
    context.main_task_list.task_list.add_task({
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
            cmd_list.begin_renderpass({
                .color_attachments = 
                {{
                    .image_view = swapchain_image[0].default_view(),
                    .load_op = daxa::AttachmentLoadOp::CLEAR,
                    .clear_value = std::array<f32, 4>{0.04, 0.02, 0.09, 1.0}
                }},
                .depth_attachment = {},
                .render_area = {.x = 0, .y = 0, .width = dimensions.x , .height = dimensions.y}
            });

            cmd_list.set_pipeline(context.pipelines.p_draw_AABB);
            cmd_list.push_constant(AABBDrawPC{
                .tmp = 35
            });
            cmd_list.draw({.vertex_count = 3});
            cmd_list.end_renderpass();
        },
        .debug_name = "display texture",
    });
}