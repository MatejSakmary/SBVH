#pragma once

#include "triangle.hpp"
#include "aabb.hpp"
#include "../types.hpp"
#include "../utils.hpp"
#include "../rendering_backend/shared/draw_aabb_shared.inl"

struct SAHCalculateInfo
{
    u32 left_primitive_count;
    u32 right_primitive_count;
    f32 left_aabb_area;
    f32 right_aabb_area;
    f32 parent_aabb_area;
    f32 ray_aabb_test_cost;
    f32 ray_tri_test_cost;
};

auto SAH(const SAHCalculateInfo & info) -> f32;

enum PointClassification
{
    ON_NEGATIVE_SIDE,
    ON_POSITIVE_SIDE,
    ON_BOUNDARY
};

enum SplitType
{
    OBJECT,
    SPATIAL,
};

struct NodeSpan 
{ 
    size_t start;
    size_t size;
};

struct PrimitiveAABB
{
    AABB aabb;
    const Triangle * primitive{};
};

struct BVHNode
{
    AABB bounding_box;
    // indices are always into the vector of bvh_nodes unless they are negative
    // in which case they are indexing into an array of scene primitives
    i32 left_index{};  
    i32 right_index{};
#ifdef VISUALIZE_SPATIAL_SPLITS
    u32 spatial;
#endif
};

struct BVHLeaf
{
    std::vector<const Triangle *> primitives;
};

struct BestSplitInfo
{
    Axis axis;
    SplitType type;
    // either the index of an object or axis coord of the splitting plane in world space
    std::variant<i32,f32> event;
    f32 cost;
    AABB left_bounding_box;
    AABB right_bounding_box;
};

struct SAHGreedySplitInfo
{
    const float ray_primitive_cost;
    const float ray_aabb_test_cost;
    const u32 & node_idx;
    const NodeSpan node_span;
    const bool join_leaves;
};

struct SpatialSplitInfo
{
    const float ray_primitive_cost;
    const float ray_aabb_test_cost;
    const u32 bin_count;
    const u32 & node_idx;
    const NodeSpan node_span;
};

struct SplitNodeInfo
{
    const BestSplitInfo & split;
    const NodeSpan node_span;
    const u32 node_idx;
    const f32 ray_primitive_intersection_cost;
    const f32 ray_aabb_intersection_cost;
};

struct ProjectPrimitiveInfo
{
    const Triangle & triangle;
    const Axis splitting_axis;
    const f32 left_plane_axis_coord;
    const f32 right_plane_axis_coord;
    const AABB & parent_aabb;
    AABB & left_aabb;
    AABB & right_aabb;
};

struct ConstructBVHInfo
{
    f32 ray_primitive_intersection_cost;
    f32 ray_aabb_intersection_cost;
    u32 spatial_bin_count;
    f32 spatial_alpha;
    bool join_leaves;
    i32 max_triangles_in_leaves;
    i32 min_depth_for_join;
};

struct BVHStats
{
    u32 triangle_count;
    u32 inner_node_count;
    u32 leaf_primitives_count;
    u32 leaf_count;
    f32 total_cost;
    f64 build_time;
};

struct CreateLeafInfo
{
    BVHStats & stats;
    u32 node_idx;
    NodeSpan node_span;
};

struct ClipAxisPlaneInfo
{
    Polygon * curr_polygon;
    const Polygon * back_polygon;
    Axis clip_axis;
    f32 clip_coord;
    bool far;
};

struct BVH
{
    static auto project_primitive_into_bin_fast(const ProjectPrimitiveInfo & info) -> void;
    static auto clip_axis_plane(const ClipAxisPlaneInfo & info) -> void;
    static auto classify_point_axis_plane(const f32vec3 & point, Axis axis, bool far, f32 coord) -> PointClassification;
    // TODO(msakmary) this is non-static only for debugging purposes, make this static later
    /*static*/ auto project_primitive_into_bin_slow(const ProjectPrimitiveInfo & info) -> void;
    [[nodiscard]] auto get_bvh_visualization_data() const -> std::vector<AABBGeometryInfo>;

    auto construct_bvh_from_data(const std::vector<Triangle> & primitives, const ConstructBVHInfo & info) -> BVHStats;
    [[nodiscard]] auto get_nearest_intersection(const Ray & ray) const -> Hit;

    private:
        using SplitPrimitives = std::pair<NodeSpan,NodeSpan>;

        auto SAH_greedy_best_split(const SAHGreedySplitInfo & info) -> BestSplitInfo;
        auto spatial_best_split(const SpatialSplitInfo & info) -> BestSplitInfo;
        auto split_node(const SplitNodeInfo & info) -> SplitPrimitives;
        auto create_leaf(const CreateLeafInfo & info) -> void;
        std::vector<PrimitiveAABB> primitive_aabbs_global;
        std::vector<BVHNode> bvh_nodes;
        std::vector<BVHLeaf> bvh_leaves;
};