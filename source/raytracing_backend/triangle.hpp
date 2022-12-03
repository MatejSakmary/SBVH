#pragma once

#include <iterator> // For std::forward_iterator_tag
#include <cstddef>  // For std::ptrdiff_t

#include "../types.hpp"

struct Triangle
{
    f32vec3 v0;
    f32vec3 v1;
    f32vec3 v2;

    f32vec3 normal;
};