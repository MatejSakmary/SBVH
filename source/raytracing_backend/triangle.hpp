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

};