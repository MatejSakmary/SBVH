#pragma once

#include "triangle.hpp"
#include "../types.hpp"
#include "../utils.hpp"
#include "../rendering_backend/shared/draw_aabb_shared.inl"

struct AABB
{
    // left_bottom_front is defined as the minimum possible coordinates in all three
    // axes. right_top_back is the oposite (maximum possible coordiantes in all three axes)
    // Example -> given cube which center is (0,0,0) and all sides are length 1
    //      left_bottom_front = {-0.5f, -0.5f, -0.5f}
    //      right_top_back    = { 0.5f,  0.5f,  0.5f}
    // NOTE(msakmary) I may have to flip z coords for the realtime visualisation to work
    f32vec3 left_bottom_front;
    f32vec3 right_top_back;

    AABB();
    AABB(const f32vec3 & left_bottom_front, const f32vec3 & right_top_back);
    // expands the min and max bounds of an AABB to contained passed vertex
    void expand_bounds(const f32vec3 & vertex);
};

struct Node
{
    AABB bounding_box;
    // indices are always into the vector of bvh_nodes unless they are negative
    // in which case they are indexing into an array of scene primitives
    i32 left_index;  
    i32 right_index;
};

struct BVH
{
    void construct_bvh_from_data(const std::vector<Triangle> & primitives);
    auto get_bvh_visualization_data() const -> std::vector<AABBGeometryInfo>;

    private:
        std::vector<Node> bvh_nodes;
};