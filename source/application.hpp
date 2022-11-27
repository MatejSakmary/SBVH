#pragma once

#include "window.hpp"
#include "types.hpp"
#include "rendering_backend/renderer.hpp"
#include "raytracing_backend/scene.hpp"

struct Application 
{
    struct AppState
    {
        union KeyTable
        {
            unsigned int data;
            struct
            {
                unsigned int W : 1 = 0;
                unsigned int A : 1 = 0;
                unsigned int S : 1 = 0;
                unsigned int D : 1 = 0;
                unsigned int Q : 1 = 0;
                unsigned int E : 1 = 0;
                unsigned int CTRL : 1 = 0;
                unsigned int SPACE : 1 = 0;
            } bits;
        };

        f32 last_frame_time = 0.0f;
        f32 delta_time = 0.0f;
        b32 minimized = false;

        b32 fly_cam = false;

        b32 first_input = true;
        f32vec2 last_mouse_pos;

        KeyTable key_table;
    };

    public:
        Application();
        ~Application();

        void main_loop();

    private:
        AppWindow window;
        AppState state;
        Renderer renderer;
        Scene scene;


        void init_window();
        void mouse_callback(f64 x, f64 y);
        void mouse_button_callback(i32 button, i32 action, i32 mods);
        void key_callback(i32 key, i32 code, i32 action, i32 mods);
        void window_resize_callback(i32 width, i32 height);
        void update_app_state();
};
