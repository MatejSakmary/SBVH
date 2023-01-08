#pragma once

#include "types.hpp"
#include <daxa/types.hpp>

// returns new vector whose each component is the minimum of the respective components in v1 and v2
inline auto component_wise_min(const f32vec3 & v1, const f32vec3 & v2) -> f32vec3
{
    return { glm::min(v1.x, v2.x), glm::min(v1.y, v2.y), glm::min(v1.z, v2.z)};
}
// returns new vector whose each component is the maximum of the respective components in v1 and v2
inline auto component_wise_max(const f32vec3 & v1, const f32vec3 & v2) -> f32vec3 
{
    return { glm::max(v1.x, v2.x), glm::max(v1.y, v2.y), glm::max(v1.z, v2.z)};
}

inline auto daxa_vec3_from_glm(const f32vec3 & vec) -> daxa::f32vec3 
{
    return {vec.x, vec.y, vec.z};
}

#ifdef LOG_DEBUG
#include <iostream>
#define DEBUG_OUT(x) (std::cout << x << std::endl)
#define DEBUG_VAR_OUT(x) (std::cout << #x << ": " << (x) << std::endl)
#else
#define DEBUG_OUT(x)
#define DEBUG_VAR_OUT(x)
#endif