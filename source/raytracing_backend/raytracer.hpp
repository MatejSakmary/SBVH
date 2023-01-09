#include "../types.hpp"
#include "../utils.hpp"
#include "../rendering_backend/camera.hpp"
#include "scene.hpp"

struct PhongInfo{
    f32vec3 light_position;
    f32vec3 hit_position;
    f32vec3 hit_normal;
    const Ray & ray;
};

struct Raytracer
{
    std::vector<f32vec3> color_buffer;

    Raytracer(const u32vec2 resolution);
    auto raytrace_scene(const Scene & scene, const Camera & camera) -> void;
    auto export_image() -> void;
    auto update_resolution(u32vec2 resolution) -> void;

    private: 
        u32vec2 resolution;

        auto ray_gen(const Scene & scene, const Ray & ray) -> f32vec3;
        auto trace_ray(const Scene & scene, const Ray & ray) -> Hit;
        auto phong(const PhongInfo & info) -> f32vec3;
};

