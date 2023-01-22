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

auto AABB::check_if_valid() const -> bool
{
    f32vec3 sizes = max_bounds - min_bounds;
    // one of the boxes size is negative
    if(sizes.x < 0.0f || sizes.y < 0.0f || sizes.z < 0.0f)
    {
        return false;
    }
    // box is either 0 size or is only a line
    if((sizes.x <= 0.0f && sizes.y <= 0.0f && sizes.z < 0.0f) ||
       (sizes.x == 0.0f && sizes.y == 0.0f) ||
       (sizes.y == 0.0f && sizes.z == 0.0f) ||
       (sizes.z == 0.0f && sizes.x == 0.0f))
    {
        return false;
    }

    return true;
}

auto AABB::get_area() const -> f32
{
    if(max_bounds == f32vec3(-INFINITY) || min_bounds == f32vec3(INFINITY)) { return 0.0f; }
    f32vec3 sizes = max_bounds - min_bounds;
    return  2 * sizes.x * sizes.y + 2 * sizes.y * sizes.z + 2 * sizes.z * sizes.x ;
}

auto AABB::ray_box_intersection(Ray ray) const -> Hit
{
    Hit result;
    result.internal_fac = 1.0f;
    result.hit = false;
    result.distance = INFINITY;
    result.normal = f32vec3(0.0f, 0.0f, 0.0f);

    f32vec3 inverted_direction = 1.0f / ray.direction;

    f32 tx1 = (min_bounds.x - ray.start.x) * inverted_direction.x;
    f32 tx2 = (max_bounds.x - ray.start.x) * inverted_direction.x;
    f32 tmin = glm::min(tx1, tx2);
    f32 tmax = glm::max(tx1, tx2);
    f32 ty1 = (min_bounds.y - ray.start.y) * inverted_direction.y;
    f32 ty2 = (max_bounds.y - ray.start.y) * inverted_direction.y;
    tmin = glm::max(tmin, glm::min(ty1, ty2));
    tmax = glm::min(tmax, glm::max(ty1, ty2));
    f32 tz1 = (min_bounds.z - ray.start.z) * inverted_direction.z;
    f32 tz2 = (max_bounds.z - ray.start.z) * inverted_direction.z;
    tmin = glm::max(tmin, glm::min(tz1, tz2));
    tmax = glm::min(tmax, glm::max(tz1, tz2));

    result.hit = false;
    if (tmax >= tmin) {
        if (tmin > 0) {
            result.distance = tmin;
            result.hit = true;
        } else if (tmax > 0) {
            result.distance = tmax;
            result.hit = true;
            result.internal_fac = -1.0;
            tmin = tmax;
        }
    }

    b32 is_x = tmin == tx1 || tmin == tx2;
    b32 is_y = tmin == ty1 || tmin == ty2;
    b32 is_z = tmin == tz1 || tmin == tz2;

    if (is_z) {
        if (ray.direction.z < 0) {
            result.normal = f32vec3(0, 0, 1);
        } else {
            result.normal = f32vec3(0, 0, -1);
        }
    } else if (is_y) {
        if (ray.direction.y < 0) {
            result.normal = f32vec3(0, 1, 0);
        } else {
            result.normal = f32vec3(0, -1, 0);
        }
    } else {
        if (ray.direction.x < 0) {
            result.normal = f32vec3(1, 0, 0);
        } else {
            result.normal = f32vec3(-1, 0, 0);
        }
    }
    result.normal *= result.internal_fac;

    return result;
}

auto AABB::contains(const f32vec3 & vertex) const -> bool
{
    // assert use of uninitizalied bin
    assert(min_bounds != f32vec3(INFINITY) && max_bounds != f32vec3(-INFINITY));
    if( vertex.x < min_bounds.x ||
        vertex.y < min_bounds.y ||
        vertex.z < min_bounds.z ||
        vertex.x > max_bounds.x ||
        vertex.y > max_bounds.y ||
        vertex.z > max_bounds.z)
    {
        return false;
    }
    return true;
}

auto AABB::contains(const Triangle & primitive) const -> bool
{
    // assert use of uninitizalied bin
    assert(min_bounds != f32vec3(INFINITY) && max_bounds != f32vec3(-INFINITY));
    for(int i = 0; i < 3; i++)
    {
        if(primitive[i].x < min_bounds.x ||
           primitive[i].y < min_bounds.y ||
           primitive[i].z < min_bounds.z ||
           primitive[i].x > max_bounds.x ||
           primitive[i].y > max_bounds.y ||
           primitive[i].z > max_bounds.z)
        {
            return false;
        }
    }
    return true;
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