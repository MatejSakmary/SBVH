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
    // NOTE(msakmary) I may have to flip z coords for the realtime visualisation to work
    f32vec3 min_bounds;
    f32vec3 max_bounds;

    AABB();
    AABB(const f32vec3 & min_bounds, const f32vec3 & max_bounds);
    explicit AABB(const Triangle & triangle);
    // expands the min and max bounds of an AABB to contained passed vertex
    auto expand_bounds(const f32vec3 & vertex) -> void;
    auto expand_bounds(const Triangle & triangle) -> void;
    auto expand_bounds(const AABB & aabb) -> void;
    auto get_area() const -> f32;

    // return the coordinate corresponding to the axis selection 
    inline float get_axis_centroid(Axis axis) const
    {
        return (min_bounds[axis] + max_bounds[axis]) / 2.0f;
    }
};