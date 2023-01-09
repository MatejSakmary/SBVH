#include "raytracer.hpp"
#include <thread>

#include <stb_image.h>
#include <stb_image_write.h>

Raytracer::Raytracer(u32vec2 resolution) : 
    resolution(resolution),
    color_buffer(resolution.x * resolution.y * 3) {}

auto Raytracer::update_resolution(u32vec2 new_resolution) -> void
{
    resolution = new_resolution;
    color_buffer.resize(new_resolution.x * new_resolution.y * 3);
}

auto Raytracer::export_image() -> void
{
    stbi_write_hdr("out.hdr", resolution.x, resolution.y, 3, reinterpret_cast<float*>(color_buffer.data()));
}

auto Raytracer::raytrace_scene(const Scene & scene, const Camera & camera) -> void
{

    const int num_threads = std::thread::hardware_concurrency() * 2;
    // const int num_threads = 1;
    std::vector<std::thread> threads;

    auto task = [&](int start, int end){
        for(int y = start; y < end; y++)
        {
            for(uint32_t x = 0; x < resolution.x; x++)
            {
                f32vec3 color = ray_gen(scene, camera.get_ray({x, y}, resolution));
                //NOTE(msakmary) flip the image along the X axis (so it's not upside down)
                color_buffer.at((resolution.y - y - 1) * resolution.x + x) = color; 
            }
        }
    };
    threads.reserve(num_threads);
    int chunk = resolution.y / num_threads;
    int remainder = resolution.y % num_threads;
    for(int i = 0; i < num_threads; i++)
    {
        if(i != num_threads - 1)
        {
            threads.push_back(std::thread(task, i * chunk, (i+1) * chunk));
        } else
        {
            threads.push_back(std::thread(task, i * chunk, ((i+1) * chunk) + remainder));
        }
    }
    for(int i = 0; i < num_threads; i++)
    {
        threads.at(i).join();
    }
    export_image();
}

auto Raytracer::phong(const PhongInfo & info) -> f32vec3
{
    f32vec3 to_light = glm::normalize(f32vec3(info.light_position) - info.hit_position);
    f32vec3 reflected_light = glm::reflect(-to_light, info.hit_normal); // -to_light = from_light

    float cos_beta = glm::max(glm::dot(reflected_light, -info.ray.direction), 0.0f); // -ray.direction = to_view
    float cos_alpha = glm::max(glm::dot(info.hit_normal, to_light), 0.0f);

    const f32 kd = 1.0f;
    const f32 ks = 0.0f;
    const f32 shininess = 0.0f;
    f32vec3 diffuse = f32vec3(kd * cos_alpha);
    f32vec3 specular = f32vec3(ks * glm::pow(cos_beta, shininess));

    return diffuse + specular;
}


auto Raytracer::ray_gen(const Scene & scene, const Ray & ray) -> f32vec3
{

    //TODO(msakmary) TEMPORARY!!
    const f32vec3 light_position = f32vec3(-11.0f, 15.0f, 7.0f);
    auto hit = trace_ray(scene, ray);
    if(!hit.hit) 
    {
        return f32vec3(0.0f, 0.0f, 0.0f);
    } 

    auto hit_position = ray.start + (ray.direction * hit.distance) + (0.005f * hit.normal);
    Ray to_light = Ray(hit_position, light_position - hit_position);
    f32 distance_to_light = glm::distance(hit_position, light_position);
    auto shadow_hit = trace_ray(scene, to_light);

    // is in shadow
    if(shadow_hit.distance > 0.0f && shadow_hit.distance < distance_to_light * 0.98f)
    {
        return f32vec3(0.0f, 0.0f, 0.0f);
    } else {
        return phong({
            .light_position = light_position,
            .hit_position = hit_position,
            .hit_normal = hit.normal,
            .ray = ray
        });
    }
}

auto Raytracer::trace_ray(const Scene & scene, const Ray & ray) -> Hit
{
    Hit closest_hit {};
    return scene.raytracing_scene.bvh.get_nearest_intersection(ray);
}