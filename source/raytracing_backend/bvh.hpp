#pragma once

#include "triangle.hpp"
#include "aabb.hpp"
#include "../types.hpp"
#include "../utils.hpp"
#include "../rendering_backend/shared/draw_aabb_shared.inl"

struct SAHCalculateInfo
{
    f32 left_primitive_count;
    f32 right_primitive_count;
    f32 left_aabb_area;
    f32 right_aabb_area;
    f32 parent_aabb_area;
    f32 ray_aabb_test_cost;
    f32 ray_tri_test_cost;
};

auto SAH(const SAHCalculateInfo & info) -> f32;

enum SplitType
{
    OBJECT,
    SPATIAL,
};

struct PrimitiveAABB
{
    AABB aabb;
    const Triangle * primitive;
};

struct BVHNode
{
    AABB bounding_box;
    // indices are always into the vector of bvh_nodes unless they are negative
    // in which case they are indexing into an array of scene primitives
    i32 left_index;  
    i32 right_index;
};

struct SplitInfo
{
    Axis axis;
    SplitType type;
    i32 event;
    f32 cost;
    const AABB left_bounding_box;
    const AABB right_bounding_box;
};

struct SAHGreedySplitInfo
{
    const float ray_primitive_cost;
    const float ray_aabb_test_cost;
    const u32 & node_idx;
    std::vector<PrimitiveAABB> & primitive_aabbs;
};

struct SplitNodeInfo
{
    const SplitInfo & split;
    std::vector<PrimitiveAABB> & primitive_aabbs;
    const u32 node_idx;
};

struct BVH
{
    auto construct_bvh_from_data(const std::vector<Triangle> & primitives) -> void;
    auto get_bvh_visualization_data() const -> std::vector<AABBGeometryInfo>;
    auto SAH_greedy_best_split(const SAHGreedySplitInfo & info) -> SplitInfo;
    auto split_node(const SplitNodeInfo & info) -> void;

    private:
        std::vector<BVHNode> bvh_nodes;
};