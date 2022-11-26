#pragma once

#include <cstring>
#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>

#include "../../types.hpp"
#include "../renderer_context.hpp"
#include "../shared/draw_aabb_shared.inl"


inline void task_fill_buffers(RendererContext & context)
{
    context.main_task_list.task_list.add_task({
        .used_buffers =
        {
            {
                context.main_task_list.buffers.t_cube_indices,
                daxa::TaskBufferAccess::HOST_TRANSFER_WRITE,
            },
            {
                context.main_task_list.buffers.t_transform_data,
                daxa::TaskBufferAccess::HOST_TRANSFER_WRITE,
            },
            {
                context.main_task_list.buffers.t_aabb_infos,
                daxa::TaskBufferAccess::HOST_TRANSFER_WRITE,
            }
        },
        .task = [&](daxa::TaskRuntime const & runtime)
        {
            auto cmd_list = runtime.get_command_list();
            auto index_buffer = runtime.get_buffers(context.main_task_list.buffers.t_cube_indices);
            auto transform_buffer = runtime.get_buffers(context.main_task_list.buffers.t_transform_data);
            auto aabbs_buffer = runtime.get_buffers(context.main_task_list.buffers.t_aabb_infos);

            #pragma region indices
            if(context.conditionals.fill_indices)
            {
                auto indices_staging_buffer = context.device.create_buffer({
                    .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
                    .size = sizeof(IndexBuffer),
                    .debug_name = "staging_atmosphere_gpu_buffer"
                });

                // copy data into staging buffer
                auto index_buffer_ptr = context.device.get_host_address_as<IndexBuffer>(indices_staging_buffer);
                memcpy(index_buffer_ptr, &context.buffers.index_buffer.cpu_buffer, sizeof(IndexBuffer));

                // copy staging buffer into gpu_buffer
                cmd_list.copy_buffer_to_buffer({
                    .src_buffer = indices_staging_buffer,
                    .dst_buffer = context.buffers.index_buffer.gpu_buffer,
                    .size = sizeof(IndexBuffer),
                });

                // destroy the stagin buffer after the copy is done
                cmd_list.destroy_buffer_deferred(indices_staging_buffer);
                context.conditionals.fill_indices = false;
            }
            #pragma endregion indices

            #pragma region transforms
            if(context.conditionals.fill_transforms)
            {
                auto transforms_staging_buffer = context.device.create_buffer({
                    .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
                    .size = sizeof(TransformData),
                    .debug_name = "staging_transforms_buffer"
                });

                auto transform_buffer_ptr = context.device.get_host_address_as<TransformData>(transforms_staging_buffer);
                memcpy(transform_buffer_ptr, &context.buffers.transforms_buffer.cpu_buffer, sizeof(TransformData));

                cmd_list.copy_buffer_to_buffer({
                    .src_buffer = transforms_staging_buffer,
                    .dst_buffer = context.buffers.transforms_buffer.gpu_buffer,
                    .size = sizeof(TransformData),
                });

                cmd_list.destroy_buffer_deferred(transforms_staging_buffer);
                context.conditionals.fill_transforms = false;
            }
            #pragma endregion transforms

            #pragma region aabbs
            if(context.conditionals.fill_aabbs)
            {
                auto aabbs_staging_buffer = context.device.create_buffer({
                    .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
                    .size = static_cast<u32>(sizeof(AABBGeometryInfo) * context.buffers.aabb_info_buffer.cpu_buffer.size()),
                    .debug_name = "staging_aabbs_buffer"
                });

                auto aabbs_buffer_ptr = context.device.get_host_address_as<AABBGeometryInfo>(aabbs_staging_buffer);
                memcpy(aabbs_buffer_ptr, context.buffers.aabb_info_buffer.cpu_buffer.data(), 
                    static_cast<u32>(sizeof(AABBGeometryInfo) * context.buffers.aabb_info_buffer.cpu_buffer.size()));

                cmd_list.copy_buffer_to_buffer({
                    .src_buffer = aabbs_staging_buffer,
                    .dst_buffer = context.buffers.aabb_info_buffer.gpu_buffer,
                    .size = static_cast<u32>(sizeof(AABBGeometryInfo) * context.buffers.aabb_info_buffer.cpu_buffer.size()),
                });

                cmd_list.destroy_buffer_deferred(aabbs_staging_buffer);
                context.conditionals.fill_aabbs = false;
            }
            #pragma endregion aabbs

        },
        .debug_name = "upload buffer data",
    });
}