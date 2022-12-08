#include "aabb.hpp"

AABB::AABB() : min_bounds(INFINITY), max_bounds(-INFINITY) {}

AABB::AABB(const f32vec3 & min_bounds, const f32vec3 & max_bounds) :
    min_bounds{min_bounds}, max_bounds{max_bounds} {}

AABB::AABB(const Triangle & triangle) : 
    min_bounds(
        component_wise_min(triangle.v0,
            component_wise_min(triangle.v1, triangle.v2))),
    max_bounds(
        component_wise_max(triangle.v0,
            component_wise_max(triangle.v1, triangle.v2)))
{
}

auto AABB::expand_bounds(const f32vec3 & vertex) -> void
{
    min_bounds = component_wise_min(min_bounds, vertex);
    max_bounds = component_wise_max(max_bounds, vertex);
};

auto AABB::expand_bounds(const Triangle & triangle) -> void
{
    min_bounds = component_wise_min(min_bounds, triangle.v0);
    min_bounds = component_wise_min(min_bounds, triangle.v1);
    min_bounds = component_wise_min(min_bounds, triangle.v2);
    max_bounds = component_wise_max(max_bounds, triangle.v0);
    max_bounds = component_wise_max(max_bounds, triangle.v1);
    max_bounds = component_wise_max(max_bounds, triangle.v2);
}

auto AABB::expand_bounds(const AABB & aabb) -> void
{
    min_bounds = component_wise_min(min_bounds, aabb.min_bounds);
    max_bounds = component_wise_max(max_bounds, aabb.max_bounds);
}

auto AABB::get_area() const -> f32
{
    f32vec3 sizes = max_bounds - min_bounds;
    return { 2 * sizes.x * sizes.y + 2 * sizes.y * sizes.z + 2 * sizes.z * sizes.x };
}