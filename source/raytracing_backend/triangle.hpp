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

    inline constexpr f32vec3 & operator [](int index)
    {
        switch(index)
        {
            case 0: { return v0; }
            case 1: { return v1; }
            case 2: { return v2; }
            default:
            {
                throw std::runtime_error("[Triangle::operator[]] out of bounds idx");
                return v0;
            }
        }
    }

    f32vec3 operator [](int index) const
    {
        switch(index)
        {
            case 0: { return v0; }
            case 1: { return v1; }
            case 2: { return v2; }
            default:
            {
                throw std::runtime_error("[Triangle::operator[]] out of bounds idx");
                return v0;
            }
        }
    }
};