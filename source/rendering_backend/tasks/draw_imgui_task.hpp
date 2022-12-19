#pragma once

#include <string>
#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>
#include <daxa/utils/imgui.hpp>
#include <imgui_impl_glfw.h>

#include "../camera.hpp"
#include "../renderer_context.hpp"
#include "../../external/imgui_file_dialog.hpp"

inline void intialize_file_dialog(ImGui::FileBrowser & file_dialog)
{
    file_dialog.SetTitle("Select scene file");
    file_dialog.SetTypeFilters({ ".fbx", ".obj" });
}

inline void ui_update(const Camera & camera, RendererContext & context)
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

    ImGui::Begin("Render controls window");
    if (ImGui::Button("Default Button", {100, 20})) { context.file_dialog.Open(); }
    ImGui::End();

    ImGui::Begin("Camera Info");
    ImGui::Text("Camera position is: ");
    ImGui::SameLine();
    ImGui::Text("%s", glm::to_string(camera.get_camera_position()).c_str());
    ImGui::End();

    context.file_dialog.Display();

    if(context.file_dialog.HasSelected())
    {
        std::cout << "Selected filename" << context.file_dialog.GetSelected().string() << std::endl;
        context.file_dialog.ClearSelected();
    }

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

