#pragma once

#include <iterator> // For std::forward_iterator_tag
#include <cstddef>  // For std::ptrdiff_t
#include <stdexcept>

#include "../types.hpp"

enum Axis : i32
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

    // Find intersection point - from PBRT - www.pbrt.org
    inline auto intersect_ray(const Ray & ray) const -> Hit
    {
        Hit hit = Hit{
            .hit = false,
            .distance = INFINITY,
            .normal = f32vec3(0.0f),
            .internal_fac = 1.0f
        };

        const f32vec3 e1 = v1 - v0;
        const f32vec3 e2 = v2 - v0;

        f32vec3 s1 = cross(ray.direction, e2);
        float divisor = dot(s1, e1);
        if (divisor == 0.0)
            return hit;

        float invDivisor = 1.0f / divisor;
        // Compute first barycentric coordinate
        f32vec3 d = ray.start - v0;
        float b1 = dot(d, s1) * invDivisor;
        if (b1 < 0.0f || b1 > 1.0f)
            return hit;
        // Compute second barycentric coordinate
        f32vec3 s2 = cross(d, e1);
        float b2 = dot(ray.direction, s2) * invDivisor;
        if (b2 < 0.0f || b1 + b2 > 1.0f)
            return hit;
        // Compute _t_ to intersection point
        float tt = dot(e2, s2) * invDivisor;
        if (tt < 0.0f) // In original algo there is tt < 0 || tt > maxT
            return hit;

        hit.hit = true;
        hit.distance = tt;
        hit.normal = normal;
        return hit;
    }

    inline constexpr auto operator [](int index) -> f32vec3 &
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

    auto operator [](int index) const -> f32vec3
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