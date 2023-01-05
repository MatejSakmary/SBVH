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
    return  2 * sizes.x * sizes.y + 2 * sizes.y * sizes.z + 2 * sizes.z * sizes.x ;
}

auto do_aabbs_intersect(const AABB & first, const AABB & second) -> bool
{
    if (first.min_bounds.x > second.max_bounds.x ||
        first.max_bounds.x < second.min_bounds.x ||
        first.min_bounds.y > second.max_bounds.y ||
        first.max_bounds.y < second.min_bounds.y ||
        first.min_bounds.z > second.max_bounds.z ||
        first.max_bounds.z < second.min_bounds.z)
    {
        return false;
    }
    return true;
}

auto get_intersection_aabb(const AABB & first, const AABB & second) -> AABB
{
    return AABB(
        {component_wise_max(first.min_bounds, second.min_bounds)},
        {component_wise_min(first.max_bounds, second.max_bounds)}
    );
}