#pragma once

#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>

#include "../utils.hpp"
#include "renderer_context.hpp"
#include "tasks/clear_present_task.hpp"


struct Renderer
{
    Renderer(daxa::NativeWindowHandle window_handle);
    ~Renderer();

    void resize();
    void draw();

    private:
        RendererContext context;
        void create_clear_present_task();
};