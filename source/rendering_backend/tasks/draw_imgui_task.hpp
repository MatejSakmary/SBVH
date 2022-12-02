#pragma once

#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>
#include <daxa/utils/imgui.hpp>
#include <imgui_impl_glfw.h>

inline void ui_update()
{
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGuiWindowFlags window_flags = 
        ImGuiWindowFlags_NoDocking  | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse   |
        ImGuiWindowFlags_NoResize   | ImGuiWindowFlags_NoMove       |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus;
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");

    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    ImGui::PopStyleVar(3);
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    ImGui::End();

    ImGui::Begin("Hallo window");
    ImGui::Text("Hallo");
    ImGui::End();

    ImGui::Begin("Docking");
    ImGui::End();

    ImGui::Render();
}

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

