#include "raytracer.hpp"
#include <thread>

#include <stb_image.h>
#include <stb_image_write.h>

static const float pscols[4 * 33] = { // 33 colors RGB
    0, 0.2298057, 0.298717966, 0.753683153, 0.03125, 0.26623388, 0.353094838, 0.801466763,
    0.0625, 0.30386891, 0.406535296, 0.84495867, 0.09375, 0.342804478, 0.458757618, 0.883725899,
    0.125, 0.38301334, 0.50941904, 0.917387822, 0.15625, 0.424369608, 0.558148092, 0.945619588,
    0.1875, 0.46666708, 0.604562568, 0.968154911, 0.21875, 0.509635204, 0.648280772, 0.98478814,
    0.25, 0.552953156, 0.688929332, 0.995375608, 0.28125, 0.596262162, 0.726149107, 0.999836203,
    0.3125, 0.639176211, 0.759599947, 0.998151185, 0.34375, 0.681291281, 0.788964712, 0.990363227,
    0.375, 0.722193294, 0.813952739, 0.976574709, 0.40625, 0.761464949, 0.834302879, 0.956945269,
    0.4375, 0.798691636, 0.849786142, 0.931688648, 0.46875, 0.833466556, 0.860207984, 0.901068838,
    0.5, 0.865395197, 0.86541021, 0.865395561, 0.53125, 0.897787179, 0.848937047, 0.820880546,
    0.5625, 0.924127593, 0.827384882, 0.774508472, 0.59375, 0.944468518, 0.800927443, 0.726736146,
    0.625, 0.958852946, 0.769767752, 0.678007945, 0.65625, 0.96732803, 0.734132809, 0.628751763,
    0.6875, 0.969954137, 0.694266682, 0.579375448, 0.71875, 0.966811177, 0.650421156, 0.530263762,
    0.75, 0.958003065, 0.602842431, 0.481775914, 0.78125, 0.943660866, 0.551750968, 0.434243684,
    0.8125, 0.923944917, 0.49730856, 0.387970225, 0.84375, 0.89904617, 0.439559467, 0.343229596,
    0.875, 0.869186849, 0.378313092, 0.300267182, 0.90625, 0.834620542, 0.312874446, 0.259301199,
    0.9375, 0.795631745, 0.24128379, 0.220525627, 0.96875, 0.752534934, 0.157246067, 0.184115123,
    1.0, 0.705673158, 0.01555616, 0.150232812};

static constexpr auto get_pseudocolor_cool_warm(f32 val, f32 minVal, f32 maxVal) -> f32vec3
{
    if (val < minVal) { val = minVal; }
    if (val > maxVal) { val = maxVal; }
    f32 ratio = (val - minVal) / (maxVal - minVal);
    i32 i = i32(ratio * 31.999);
    assert(i < 33);
    assert(i >= 0);
    f32 alpha = i + 1.0 - (ratio * 31.999);
    f32 r = pscols[4 * i + 1] * alpha + pscols[4 * (i + 1) + 1] * (1.0 - alpha);
    f32 g = pscols[4 * i + 2] * alpha + pscols[4 * (i + 1) + 2] * (1.0 - alpha);
    f32 b = pscols[4 * i + 3] * alpha + pscols[4 * (i + 1) + 3] * (1.0 - alpha);
    return f32vec3(r, g, b);
}

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

auto Raytracer::raytrace_scene(const Scene & scene, const Camera & camera) -> f64
{
#ifdef TRACK_TRAVERSE_STEP_COUNT
    avg_traversal_steps = 0.0f;
#endif
    // const int num_threads = std::thread::hardware_concurrency() * 2;
    const int num_threads = 1;
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
    auto start_time = std::chrono::high_resolution_clock::now();
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
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> ms_double = end_time - start_time;
    export_image();

#ifdef TRACK_TRAVERSE_STEP_COUNT
    DEBUG_OUT(
        "min traversal steps: " + std::to_string(min_traversal_steps) +
        " max traversal steps: " + std::to_string(max_traversal_steps) +
        " avg traversal steps: " + std::to_string(avg_traversal_steps)
    );
#endif
    return ms_double.count();
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
    const f32vec3 light_position = scene.light_position;
    auto hit = trace_ray(scene, ray);

#ifdef TRACK_TRAVERSE_STEP_COUNT
    avg_traversal_steps += f32(hit.traversal_steps) / f32(resolution.x * resolution.y);
    min_traversal_steps = glm::min(min_traversal_steps, hit.traversal_steps);
    max_traversal_steps = glm::max(max_traversal_steps, hit.traversal_steps);
#endif

    if(!hit.hit) 
    {
        return f32vec3(0.0f, 0.0f, 0.0f);
    } 
    //else 
    //{
        //return (hit.normal + f32vec3(1.0f)) * 0.5f;
    //}
#ifdef TRACK_TRAVERSE_STEP_COUNT
    else 
    {
        return get_pseudocolor_cool_warm(hit.traversal_steps, 1, 400);
    }
#endif


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