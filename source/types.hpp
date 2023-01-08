#pragma once
#include <cstdint>
#include <variant>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using usize = std::size_t;
using b32 = u32;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using isize = std::ptrdiff_t;

using f32 = float;
using f64 = double;

using f32vec2 = glm::vec2;
using f32vec3 = glm::vec3;
using f32vec4 = glm::vec4;

using f64vec2 = glm::dvec2;
using f64vec3 = glm::dvec3;
using f64vec4 = glm::dvec4;

using u32vec2 = glm::uvec2;
using u32vec3 = glm::uvec3;
using u32vec4 = glm::uvec4;

using i32vec2 = glm::ivec2;
using i32vec3 = glm::ivec3;
using i32vec4 = glm::ivec4;

using f32mat4x4 = glm::mat4x4;

enum Direction
{
    FORWARD,
    BACK,
    LEFT,
    RIGHT,
    UP,
    DOWN,
    ROLL_LEFT,
    ROLL_RIGHT,
    UNKNOWN
};

struct Hit
{
    bool hit;
    f32 distance;
    f32vec3 normal;
    f32 internal_fac;
};

struct Ray
{
    f32vec3 start = {0.0, 0.0, 0.0};
    f32vec3 direction = {0.0, 0.0, 0.0};

    Ray(const f32vec3 & start, const f32vec3 & direction) :
        start{start}, direction{glm::normalize(direction)} 
    {
    }
};

const f32 EPSILON = 1.0e-9;