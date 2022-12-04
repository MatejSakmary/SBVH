#include "application.hpp"
#include <bit>

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
        renderer.camera.update_front_vector(x_offset, y_offset);
    }
}

void Application::mouse_button_callback(const i32 button, const i32 action, const i32 mods)
{

}

void Application::window_resize_callback(const i32 width, const i32 height)
{
    renderer.resize();
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
            case GLFW_KEY_LEFT_CONTROL: state.key_table.bits.CTRL = update_state(action); return;
            case GLFW_KEY_SPACE: state.key_table.bits.SPACE = update_state(action); return;
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

    if(key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        renderer.reload_scene_data(scene);
    }
    return;
}

Application::Application() : 
    window({1080, 720},
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
    state{ .minimized = false },
    renderer{window},
    scene{"resources/scenes/suzanne_fbx/suzanne.fbx"}
{
}

Application::~Application()
{
}

void Application::update_app_state()
{
    f32 this_frame_time = glfwGetTime();
    state.delta_time =  this_frame_time - state.last_frame_time;
    state.last_frame_time = this_frame_time;

    if(state.key_table.data > 0)
    {
        if(state.key_table.bits.W) renderer.camera.move_camera(state.delta_time, Direction::FORWARD);
        if(state.key_table.bits.A) renderer.camera.move_camera(state.delta_time, Direction::LEFT);
        if(state.key_table.bits.S) renderer.camera.move_camera(state.delta_time, Direction::BACK);
        if(state.key_table.bits.D) renderer.camera.move_camera(state.delta_time, Direction::RIGHT);
        if(state.key_table.bits.Q) renderer.camera.move_camera(state.delta_time, Direction::ROLL_LEFT);
        if(state.key_table.bits.E) renderer.camera.move_camera(state.delta_time, Direction::ROLL_RIGHT);
        if(state.key_table.bits.CTRL) renderer.camera.move_camera(state.delta_time, Direction::DOWN);
        if(state.key_table.bits.SPACE) renderer.camera.move_camera(state.delta_time, Direction::UP);
    }
}

void Application::main_loop()
{
    while (!window.get_window_should_close())
    {
        glfwPollEvents();
        update_app_state();

        if (state.minimized) { DEBUG_OUT("[Application::main_loop()] Window minimized "); continue; } 
        renderer.draw();
    }
}
