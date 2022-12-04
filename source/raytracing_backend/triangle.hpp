#pragma once

#include <iterator> // For std::forward_iterator_tag
#include <cstddef>  // For std::ptrdiff_t

#include "../types.hpp"

enum Axis : u32
{
    X = 0,
    Y = 1, 
    Z = 2,
    LAST = 3,
};

struct Triangle
{
    f32vec3 v0;
    f32vec3 v1;
    f32vec3 v2;

    f32vec3 normal;

    // return the coordinate corresponding to the axis selection of the triangles
    // aabb
    inline float get_aabb_axis_centroid(Axis axis) const
    {
        f32 min_bounds = glm::min(glm::min(v0[axis], v1[axis]), v2[axis]);
        f32 max_bounds = glm::max(glm::max(v0[axis], v1[axis]), v2[axis]);
        return (min_bounds + max_bounds) / 2.0f;
    }
};