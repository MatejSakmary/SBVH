#pragma once

#include <functional>
#include <imgui.h>
#include <string>
#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>
#include <daxa/utils/imgui.hpp>
#include <imgui_impl_glfw.h>

#include "../camera.hpp"
#include "../renderer_context.hpp"
#include "../../external/imgui_file_dialog.hpp"

inline void task_draw_imgui(RendererContext & context)
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
            auto swapchain_image_dimensions = context.swapchain.get_surface_extent();
            context.imgui_renderer.record_commands(
                ImGui::GetDrawData(),
                cmd_list, context.swapchain_image,
                swapchain_image_dimensions.x, swapchain_image_dimensions.y);
        },
        .debug_name = "Imgui Task",
    });
}

