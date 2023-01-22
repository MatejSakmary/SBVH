#include "triangle.hpp"
#include "../types.hpp"
#include "../utils.hpp"

struct AABB
{
    // min_bounds is defined as the minimum possible coordinates in all three
    // axes. max_bounds is the oposite (maximum possible coordiantes in all three axes)
    // Example -> given cube which center is (0,0,0) and all sides are length 1
    //      min_bounds = left_bottom_front = {-0.5f, -0.5f, -0.5f}
    //      max_boudns = right_top_back    = { 0.5f,  0.5f,  0.5f}
    f32vec3 min_bounds;
    f32vec3 max_bounds;

    AABB();
    AABB(const f32vec3 & min_bounds, const f32vec3 & max_bounds);
    explicit AABB(const Triangle & triangle);
    // expands the min and max bounds of an AABB to contained passed vertex
    auto expand_bounds(const f32vec3 & vertex) -> void;
    auto expand_bounds(const Triangle & triangle) -> void;
    auto expand_bounds(const AABB & aabb) -> void;
    [[nodiscard]] auto ray_box_intersection(Ray ray) const -> Hit;
    [[nodiscard]] auto check_if_valid() const -> bool;
    [[nodiscard]] auto get_area() const -> f32;
    [[nodiscard]] auto contains(const Triangle & primitive) const -> bool;
    [[nodiscard]] auto contains(const f32vec3 & vertex) const -> bool;

    // return the coordinate corresponding to the axis selection 
    [[nodiscard]] inline auto get_axis_centroid(Axis axis) const -> float
    {
        return (min_bounds[axis] + max_bounds[axis]) / 2.0f;
    }
};

auto do_aabbs_intersect(const AABB & first, const AABB & second) -> bool;
auto get_intersection_aabb(const AABB & first, const AABB & second) -> AABB;