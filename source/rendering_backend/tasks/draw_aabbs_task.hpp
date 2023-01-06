#pragma once

#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>
#include <daxa/utils/pipeline_manager.hpp>

#include "../../types.hpp"
#include "../renderer_context.hpp"
#include "../shared/draw_aabb_shared.inl"

inline static const daxa::RasterPipelineCompileInfo DRAW_AABB_TASK_RASTER_PIPE_INFO 
{
    .vertex_shader_info = {
        .source = daxa::ShaderFile{"aabb.glsl"},
        .compile_options = {
            .defines = {
                {"_VERTEX", ""},
#ifdef VISUALIZE_SPATIAL_SPLITS
                {"VISUALIZE_SPATIAL_SPLITS", ""},
#endif
            },
        },
    },
    .fragment_shader_info = {
        .source = daxa::ShaderFile{"aabb.glsl"},
        .compile_options = {
            .defines = {
                { "_FRAGMENT", ""},
#ifdef VISUALIZE_SPATIAL_SPLITS
                {"VISUALIZE_SPATIAL_SPLITS", ""},
#endif
            },
        },
    },
    .color_attachments = {
        daxa::RenderAttachment{
            .format = daxa::Format::B8G8R8A8_SRGB,
        },
    },
    .depth_test = {
        .depth_attachment_format = daxa::Format::D32_SFLOAT,
        .enable_depth_test = true,
        .enable_depth_write = true,
    },
    .raster = {
        .primitive_topology = daxa::PrimitiveTopology::LINE_STRIP,
        .primitive_restart_enable = true,
        .polygon_mode = daxa::PolygonMode::LINE
    },
    .push_constant_size = sizeof(AABBDrawPC),
};

inline void task_draw_AABB(RendererContext & context)
{
    context.main_task_list.task_list.add_task({
        .used_buffers =
        {
            {
                context.main_task_list.buffers.t_cube_indices,
                daxa::TaskBufferAccess::SHADER_READ_ONLY,
            },
            {
                context.main_task_list.buffers.t_transform_data,
                daxa::TaskBufferAccess::SHADER_READ_ONLY,
            },
            {
                context.main_task_list.buffers.t_aabb_infos,
                daxa::TaskBufferAccess::SHADER_READ_ONLY,
            }
        },
        .used_images =
        {
            { 
                context.main_task_list.images.t_swapchain_image,
                daxa::TaskImageAccess::SHADER_WRITE_ONLY,
                daxa::ImageMipArraySlice{} 
            },
            { 
                context.main_task_list.images.t_depth_image,
                daxa::TaskImageAccess::DEPTH_ATTACHMENT,
                daxa::ImageMipArraySlice{.image_aspect = daxa::ImageAspectFlagBits::DEPTH} 
            }
        },
        .task = [&](daxa::TaskRuntime const & runtime)
        {
            if(!context.buffers.aabb_info_buffer.cpu_buffer.empty())
            {
                auto cmd_list = runtime.get_command_list();
                auto dimensions = context.swapchain.get_surface_extent();
                auto swapchain_image = runtime.get_images(context.main_task_list.images.t_swapchain_image);
                auto index_buffer = runtime.get_buffers(context.main_task_list.buffers.t_cube_indices);
                auto depth_image = runtime.get_images(context.main_task_list.images.t_depth_image);
                auto transforms_buffer = runtime.get_buffers(context.main_task_list.buffers.t_transform_data);
                auto aabbs_buffer = runtime.get_buffers(context.main_task_list.buffers.t_aabb_infos);
                cmd_list.begin_renderpass({
                    .color_attachments = 
                    {{
                        .image_view = swapchain_image[0].default_view(),
                        .load_op = daxa::AttachmentLoadOp::LOAD,
                    }},
                    .depth_attachment = 
                    {{
                        .image_view = depth_image[0].default_view(),
                        .layout = daxa::ImageLayout::ATTACHMENT_OPTIMAL,
                        .load_op = daxa::AttachmentLoadOp::LOAD,
                        .store_op = daxa::AttachmentStoreOp::STORE,
                    }},
                    .render_area = {.x = 0, .y = 0, .width = dimensions.x , .height = dimensions.y}
                });

                auto m_model = glm::mat4x4(1.0f);
                cmd_list.set_pipeline(*context.pipelines.p_draw_AABB);
                cmd_list.push_constant(AABBDrawPC{
                    .transforms = context.device.get_device_address(transforms_buffer[0]),
                    .aabb_transforms = context.device.get_device_address(aabbs_buffer[0]),
                    .bvh_visualization_depth = context.render_info.visualized_depth,
                });
                cmd_list.set_index_buffer(index_buffer[0], 0, sizeof(u32));
                cmd_list.draw_indexed({
                    .index_count = INDEX_COUNT,
                    .instance_count = static_cast<u32>(context.buffers.aabb_info_buffer.cpu_buffer.size())
                });
                cmd_list.end_renderpass();
            }
        },
        .debug_name = "draw AABB",
    });
}