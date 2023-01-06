#pragma once

#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>
#include <daxa/utils/imgui.hpp>
#include <daxa/utils/pipeline_manager.hpp>

#include "../utils.hpp"
#include "camera.hpp"
#include "renderer_context.hpp"
// TODO(msakmary) rework the header structure
#include "../raytracing_backend/scene.hpp"
#include "../window.hpp"
#include "tasks/draw_aabbs_task.hpp"
#include "tasks/draw_scene_task.hpp"
#include "tasks/draw_imgui_task.hpp"
#include "tasks/fill_buffers_task.hpp"

struct Renderer
{

    explicit Renderer(const AppWindow & window);
    ~Renderer();

    void resize();
    void draw(const Camera & camera);
    void reload_scene_data(const Scene & scene);
    void reload_bvh_data(const BVH & bvh);
    void set_bvh_visualization_depth(i32 depth);

    private:
        RendererContext context;
        void create_main_task();
};