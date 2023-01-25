#include "application.hpp"
#include <bit>

#include "raytracing_backend/scene.hpp"
#include "utils.hpp"

void Application::mouse_callback(const f64 x, const f64 y)
{
    f32 x_offset;
    f32 y_offset;
    if(!state.first_input)
    {
        x_offset = x - state.last_mouse_pos.x;
        y_offset = y - state.last_mouse_pos.y;
    } else {
        x_offset = 0.0f;
        y_offset = 0.0f;
        state.first_input = false;
    }

    if(state.fly_cam)
    {
        state.last_mouse_pos = {x, y};
        camera.update_front_vector(x_offset, y_offset);
    }
}

void Application::mouse_button_callback(const i32 button, const i32 action, const i32 mods)
{

}

void Application::window_resize_callback(const i32 width, const i32 height)
{
    renderer.resize();
    raytracer.update_resolution(u32vec2{u32(width), u32(height)});
    camera.aspect_ratio = f32(width) / f32(height);
}

void Application::key_callback(const i32 key, const i32 code, const i32 action, const i32 mods)
{
    if(action == GLFW_PRESS || action == GLFW_RELEASE)
    {
        auto update_state = [](i32 action) -> unsigned int
        {
            if(action == GLFW_PRESS) return 1;
            return 0;
        };

        switch (key)
        {
            case GLFW_KEY_W: state.key_table.bits.W = update_state(action); return;
            case GLFW_KEY_A: state.key_table.bits.A = update_state(action); return;
            case GLFW_KEY_S: state.key_table.bits.S = update_state(action); return;
            case GLFW_KEY_D: state.key_table.bits.D = update_state(action); return;
            case GLFW_KEY_Q: state.key_table.bits.Q = update_state(action); return;
            case GLFW_KEY_E: state.key_table.bits.E = update_state(action); return;
            case GLFW_KEY_SPACE: state.key_table.bits.SPACE = update_state(action); return;
            case GLFW_KEY_LEFT_SHIFT: state.key_table.bits.LEFT_SHIFT = update_state(action); return;
            case GLFW_KEY_LEFT_CONTROL: state.key_table.bits.CTRL = update_state(action); return;
            default: break;
        }
    }

    if(key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        state.fly_cam = !state.fly_cam;
        if(state.fly_cam)
        {
            window.set_input_mode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            state.first_input = true;
        } else {
            window.set_input_mode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void Application::ui_update()
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
    if (ImGui::Button("Reload Scene", {100, 20})) { state.scene_file_browser.Open(); }
    if (ImGui::Button("Raytrace View", {100, 20})) 
    { 
        scene.light_position = state.light_position;
        state.raytrace_time = raytracer.raytrace_scene(scene, camera); 
    }
    f32 light_pos[] = {state.light_position.x, state.light_position.y, state.light_position.z};
    ImGui::SliderFloat3("Light position", light_pos, 0.0f, 10000.0f);
    state.light_position.x = light_pos[0];
    state.light_position.y = light_pos[1];
    state.light_position.z = light_pos[2];

    ImGui::InputInt("BVH depth", &state.visualized_depth, 1, 10);
    renderer.set_bvh_visualization_depth(state.visualized_depth);
    ImGui::End();

    ImGui::Begin("Info");
    ImGui::Text("Camera position is: ");
    ImGui::SameLine();
    ImGui::Text("%s", glm::to_string(camera.get_camera_position()).c_str());

    ImGui::InputFloat3("New camera pos: ", reinterpret_cast<f32*>(&state.camera_info.position));
    ImGui::InputFloat3("New camera dir: ", reinterpret_cast<f32*>(&state.camera_info.front));
    ImGui::InputFloat3("New camera up: ", reinterpret_cast<f32*>(&state.camera_info.up));
    ImGui::InputFloat("New camera angle: ", &state.camera_info.fov);

    if(ImGui::Button("Set Camera Params", {150, 20})) { camera.set_info(state.camera_info);}
    ImGui::SameLine();
    if(ImGui::Button("Parse view file", {150, 20})) { state.view_file_browser.Open(); }

    ImGui::Separator();
    ImGui::Text("triangle count : %u", state.bvh_stats.triangle_count);
    ImGui::Text("inner node_count : %u", state.bvh_stats.inner_node_count);
    ImGui::Text("leaf primitives_count : %u", state.bvh_stats.leaf_primitives_count);
    ImGui::Text("leaf count : %u", state.bvh_stats.leaf_count);
    ImGui::Text("average leaf depth : %f", state.bvh_stats.average_leaf_depth);
    ImGui::Text("average primitives in leaf: %f", state.bvh_stats.average_primitives_in_leaf);
    ImGui::Text("bvh max depth: %u", state.bvh_stats.max_tree_depth);
    ImGui::Text("total cost : %.3f", state.bvh_stats.total_cost);
    ImGui::Text("build time : %.3f ms", state.bvh_stats.build_time);
    ImGui::Separator();
    ImGui::Text("raytrace time : %.3f ms", state.raytrace_time);
    ImGui::End();

    ImGui::Begin("BVH build parameters");
    i32 slider_tmp = state.bvh_info.spatial_bin_count;
    ImGui::InputFloat("Ray-triangle cost", &state.bvh_info.ray_primitive_intersection_cost);
    ImGui::InputFloat("Ray-AABB cost", &state.bvh_info.ray_aabb_intersection_cost);
    ImGui::SliderInt("Spatial Splits", &slider_tmp, 1, 256);
    ImGui::InputFloat("Spatial alpha", &state.bvh_info.spatial_alpha, 0.0001f, 0.001f, "%.6f");
    ImGui::Checkbox("Join leaves", &state.bvh_info.join_leaves);

    if(!state.bvh_info.join_leaves) { ImGui::BeginDisabled(); }
    ImGui::InputInt("Max triangles in leaf", &state.bvh_info.max_triangles_in_leaves);
    ImGui::InputInt("Min join depth", &state.bvh_info.min_depth_for_join);
    if(!state.bvh_info.join_leaves) { ImGui::EndDisabled(); }

    if (ImGui::Button("Rebuild BVH", {100, 20})) { rebuild_bvh(state.bvh_info); }
    ImGui::End();

    state.bvh_info.spatial_bin_count = slider_tmp;

    state.scene_file_browser.Display();
    state.view_file_browser.Display();

    if(state.scene_file_browser.HasSelected())
    {
        reload_scene(state.scene_file_browser.GetSelected().string());
        state.scene_file_browser.ClearSelected();
    }
    if(state.view_file_browser.HasSelected())
    {
        camera.parse_view_file(state.view_file_browser.GetSelected().string());
        state.view_file_browser.ClearSelected();
    }

    ImGui::Render();
}

Application::Application() : 
    window({800, 800},
    WindowVTable {
        .mouse_pos_callback = [this](const f64 x, const f64 y)
            {this->mouse_callback(x, y);},
        .mouse_button_callback = [this](const i32 button, const i32 action, const i32 mods)
            {this->mouse_button_callback(button, action, mods);},
        .key_callback = [this](const i32 key, const i32 code, const i32 action, const i32 mods)
            {this->key_callback(key, code, action, mods);},
        .window_resized_callback = [this](const i32 width, const i32 height)
            {this->window_resize_callback(width, height);},
    }),
    state{ 
        .minimized = 0u,
        .scene_file_browser = ImGui::FileBrowser(ImGuiFileBrowserFlags_NoModal),
        .view_file_browser = ImGui::FileBrowser(ImGuiFileBrowserFlags_NoModal),
        .bvh_info = ConstructBVHInfo{
            .ray_primitive_intersection_cost = 2.0f,
            .ray_aabb_intersection_cost = 3.0f,
            .spatial_bin_count = 8,
            .spatial_alpha = 10e-5,
            .join_leaves = true
        }
    },
    renderer{window},
    camera {{
        .position = {0.0, 0.0, 500.0},
        .front = {0.0, 0.0, -1.0},
        .up = {0.0, 1.0, 0.0}, 
        .aspect_ratio = 800.0f/800.0f,
        .fov = glm::radians(50.0f)
    }},
    scene{"resources/scenes/cubes/cubes.fbx"},
    raytracer{{800, 800}}
{
    state.view_file_browser.SetTitle("Select view file");
    state.view_file_browser.SetTypeFilters({ ".view", ".VIEW" });

    state.scene_file_browser.SetTitle("Select scene file");
    state.scene_file_browser.SetTypeFilters({ ".fbx", ".obj", ".bin" });
}

void Application::reload_scene(const std::string & path)
{
    scene = Scene(path);
    state.bvh_stats = {};
    state.raytrace_time = 0.0;
    renderer.reload_scene_data(scene);
    renderer.reload_bvh_data(scene.raytracing_scene.bvh);
}

void Application::rebuild_bvh(const ConstructBVHInfo & info)
{
    state.bvh_stats = scene.build_bvh(info);
    renderer.reload_bvh_data(scene.raytracing_scene.bvh);
}

void Application::update_app_state()
{
    f64 this_frame_time = glfwGetTime();
    state.delta_time =  this_frame_time - state.last_frame_time;
    state.last_frame_time = this_frame_time;

    if(state.key_table.data > 0 && state.fly_cam == true)
    {
        if(state.key_table.bits.W)      { camera.move_camera(state.delta_time, Direction::FORWARD);    }
        if(state.key_table.bits.A)      { camera.move_camera(state.delta_time, Direction::LEFT);       }
        if(state.key_table.bits.S)      { camera.move_camera(state.delta_time, Direction::BACK);       }
        if(state.key_table.bits.D)      { camera.move_camera(state.delta_time, Direction::RIGHT);      }
        if(state.key_table.bits.Q)      { camera.move_camera(state.delta_time, Direction::ROLL_LEFT);  }
        if(state.key_table.bits.E)      { camera.move_camera(state.delta_time, Direction::ROLL_RIGHT); }
        if(state.key_table.bits.CTRL)   { camera.move_camera(state.delta_time, Direction::DOWN);       }
        if(state.key_table.bits.SPACE)  { camera.move_camera(state.delta_time, Direction::UP);         }
    }
}

void Application::main_loop()
{
    while (!window.get_window_should_close())
    {
        glfwPollEvents();
        update_app_state();
        ui_update();

        if (state.minimized != 0u) { DEBUG_OUT("[Application::main_loop()] Window minimized "); continue; } 
        renderer.draw(camera);
    }
}
