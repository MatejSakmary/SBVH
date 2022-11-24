#include "application.hpp"
#include "utils.hpp"

void Application::mouse_callback(f64 x, f64 y)
{

}

void Application::mouse_button_callback(i32 button, i32 action, i32 mods)
{

}

void Application::window_resize_callback(i32 width, i32 height)
{
}

void Application::key_callback(i32 key, i32 code, i32 action, i32 mods)
{
    if(key == GLFW_KEY_ENTER && action == GLFW_PRESS)
    {
        DEBUG_OUT("[Application::key_callback] ENTER PRESSED");
    }
    return;
}

Application::Application() : 
    window({1080, 720},
    WindowVTable {
        .mouse_pos_callback = std::bind(
            &Application::mouse_callback, this,
            std::placeholders::_1,
            std::placeholders::_2),
        .mouse_button_callback = std::bind(
            &Application::mouse_button_callback, this,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3),
        .key_callback = std::bind(
            &Application::key_callback, this,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            std::placeholders::_4),
        .window_resized_callback = std::bind(
            &Application::window_resize_callback, this,
            std::placeholders::_1,
            std::placeholders::_2)
        }
    ),
    state{ .minimized = false }
{
}

Application::~Application()
{
}

void Application::main_loop()
{
    while (!window.get_window_should_close())
    {
       glfwPollEvents();
       if (state.minimized) { DEBUG_OUT("[Application::main_loop()] Window minimized "); continue; } 
    }
}
