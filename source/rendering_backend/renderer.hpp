#pragma once

#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>

#include "../utils.hpp"
#include "camera.hpp"
#include "renderer_context.hpp"
// TODO(msakmary) rework the header structure
#include "../raytracing_backend/scene.hpp"
#include "tasks/draw_aabbs_task.hpp"
#include "tasks/draw_scene_task.hpp"
#include "tasks/fill_buffers_task.hpp"

struct Renderer
{
    Camera camera;

    Renderer(daxa::NativeWindowHandle window_handle);
    ~Renderer();

    void resize();
    void draw();
    void reload_scene_data(const Scene & scene);

    private:
        RendererContext context;
        void create_main_task();
};