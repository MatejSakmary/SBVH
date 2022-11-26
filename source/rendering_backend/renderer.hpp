#pragma once

#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>

#include "../utils.hpp"
#include "camera.hpp"
#include "renderer_context.hpp"
#include "tasks/draw_aabbs_task.hpp"
#include "tasks/fill_index_buffer_task.hpp"

struct Renderer
{
    Camera camera;

    Renderer(daxa::NativeWindowHandle window_handle);
    ~Renderer();

    void resize();
    void draw();

    private:
        RendererContext context;
        void create_main_task();
};