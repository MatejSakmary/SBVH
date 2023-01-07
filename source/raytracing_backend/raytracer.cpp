#include "raytracer.hpp"
#include <stb_image.h>
#include <stb_image_write.h>

Raytracer::Raytracer(u32vec2 resolution) : 
    resolution(resolution),
    color_buffer(resolution.x * resolution.y * 3) {}

auto Raytracer::export_image() -> void
{
    stbi_write_hdr("out.hdr", resolution.x, resolution.y, 3, reinterpret_cast<float*>(color_buffer.data()));
}

auto Raytracer::raytrace_scene(const Scene & scene, const Camera & camera) -> void
{
    for(u32 y = 0; y < resolution.y; y++)
    {
        std::cout << "progress " << u32((f32(y)/f32(resolution.y)) * 100.0f) << "%\r" << std::flush; 
        for(u32 x = 0; x < resolution.x; x++)
        {
            f32vec3 color = ray_gen(scene, camera.get_ray({x, y}, resolution));
            color_buffer.at(y * resolution.x + x) = color; 
        }
    }
    export_image();
}

auto Raytracer::ray_gen(const Scene & scene, const Ray & ray) -> f32vec3
{
    auto hit = trace_ray(scene, ray);
    if(hit.hit) { return f32vec3(1.0f, 0.0f, 0.0f); } 
    else        { return f32vec3(0.0f, 0.0f, 0.0f); }
}

auto Raytracer::trace_ray(const Scene & scene, const Ray & ray) -> Hit
{
    Hit closest_hit {};
    return scene.raytracing_scene.bvh.get_nearest_intersection(ray);
}