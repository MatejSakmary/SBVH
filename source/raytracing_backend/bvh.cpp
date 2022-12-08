#include "bvh.hpp"

#include <algorithm>

AABB::AABB() : min_bounds(INFINITY), max_bounds(-INFINITY) {}

AABB::AABB(const f32vec3 & min_bounds, const f32vec3 & max_bounds) :
    min_bounds{min_bounds}, max_bounds{max_bounds} {}

AABB::AABB(const Triangle & triangle) : 
    min_bounds(
        component_wise_min(triangle.v0,
            component_wise_min(triangle.v1, triangle.v2))),
    max_bounds(
        component_wise_max(triangle.v0,
            component_wise_max(triangle.v1, triangle.v2)))
{
}

auto AABB::expand_bounds(const f32vec3 & vertex) -> void
{
    min_bounds = component_wise_min(min_bounds, vertex);
    max_bounds = component_wise_max(max_bounds, vertex);
};

auto AABB::expand_bounds(const Triangle & triangle) -> void
{
    min_bounds = component_wise_min(min_bounds, triangle.v0);
    min_bounds = component_wise_min(min_bounds, triangle.v1);
    min_bounds = component_wise_min(min_bounds, triangle.v2);
    max_bounds = component_wise_max(max_bounds, triangle.v0);
    max_bounds = component_wise_max(max_bounds, triangle.v1);
    max_bounds = component_wise_max(max_bounds, triangle.v2);
}

auto AABB::expand_bounds(const AABB & aabb) -> void
{
    min_bounds = component_wise_min(min_bounds, aabb.min_bounds);
    max_bounds = component_wise_max(max_bounds, aabb.max_bounds);
}

auto AABB::get_area() const -> f32
{
    f32vec3 sizes = max_bounds - min_bounds;
    return { 2 * sizes.x * sizes.y + 2 * sizes.y * sizes.z + 2 * sizes.z * sizes.x };
}


auto BVH::construct_bvh_from_data(const std::vector<Triangle> & primitives) -> void
{
    // Generate vector of AABBs from the vector of primitives
    std::vector<AABB> primitive_aabbs;
    primitive_aabbs.reserve(primitives.size());
    auto primitives_it = primitives.begin();
    std::generate_n(std::back_inserter(primitive_aabbs), primitives.size(), [&]{ return AABB(*(primitives_it++));});

    // Calculate the AABB of the scene
    const float leaf_create_cost = 2.0f;
    const u32 root_node_idx = 0;
    bvh_nodes.emplace_back();
    std::for_each(primitive_aabbs.begin(), primitive_aabbs.end(), [&](const AABB & aabb)
        {bvh_nodes.at(root_node_idx).bounding_box.expand_bounds(aabb);});

    // split algorithm
    f32 best_cost = primitive_aabbs.size() * leaf_create_cost * bvh_nodes.at(root_node_idx).bounding_box.get_area();
    Axis best_axis = Axis::LAST;
    i32 best_event = -1;

    for(u32 axis = Axis::X; axis < Axis::LAST; axis++)
    {
        auto compare_op = [axis](const AABB & first, const AABB & second) -> bool
        {
            auto centroid_first = first.get_axis_centroid(static_cast<Axis>(axis));
            auto centroid_second = second.get_axis_centroid(static_cast<Axis>(axis));
            return centroid_first < centroid_second;
        };
        std::sort(primitive_aabbs.begin(), primitive_aabbs.end(), compare_op);

        // left sweep
        std::vector<f32> left_sweep_areas(primitive_aabbs.size(), 0.0);
        AABB left_sweep_aabb;
        for(size_t i = 0; i < primitive_aabbs.size(); i++)
        {
            left_sweep_aabb.expand_bounds(primitive_aabbs.at(i));
            left_sweep_areas.at(i) = left_sweep_aabb.get_area();
        }

        // right sweep
        AABB right_sweep_aabb;
        for(i32 i = primitive_aabbs.size() - 1; i >= 0 ; i--)
        {
            right_sweep_aabb.expand_bounds(primitive_aabbs.at(i));
            f32 cost = SAH({
                .left_primitive_count = static_cast<f32>(i),
                .right_primitive_count = primitive_aabbs.size() - static_cast<f32>(i),
                .left_aabb_area = left_sweep_areas.at(i),
                .right_aabb_area = right_sweep_aabb.get_area(),
                .parent_aabb_area = bvh_nodes.at(root_node_idx).bounding_box.get_area(),
                .ray_aabb_test_cost = 3.0f,
                .ray_tri_test_cost = 2.0f
            });
            if(cost < best_cost)
            {
                best_cost = cost;
                best_event = i;
                best_axis = static_cast<Axis>(axis);
            }
        }
    }
    DEBUG_VAR_OUT(best_cost);
    DEBUG_VAR_OUT(best_axis);
    DEBUG_VAR_OUT(best_event);

    auto & left_child = bvh_nodes.emplace_back();
    bvh_nodes.at(root_node_idx).left_index = bvh_nodes.size() - 1;
    for(int i = 0; i < best_event; i++)
    {
        left_child.bounding_box.expand_bounds(primitive_aabbs.at(i));
    }

    auto & right_child = bvh_nodes.emplace_back();
    bvh_nodes.at(root_node_idx).right_index = bvh_nodes.size() - 1;
    for(int i = best_event; i < primitive_aabbs.size(); i++)
    {
        right_child.bounding_box.expand_bounds(primitive_aabbs.at(i));
    }
}

auto BVH::get_bvh_visualization_data() const -> std::vector<AABBGeometryInfo>
{
    std::vector<AABBGeometryInfo> info;
    info.reserve(bvh_nodes.size());

    float depth = 0;
    for(const auto & node : bvh_nodes)
    {
        const auto & aabb = node.bounding_box;
        info.emplace_back(AABBGeometryInfo{
            .position = daxa_vec3_from_glm(aabb.min_bounds),
            .scale = daxa_vec3_from_glm(aabb.max_bounds - aabb.min_bounds),
            .depth = static_cast<daxa::f32>(depth)
        });
        if(depth == 0.0f) {depth += 1.0f;}
    }
    return info;
}

auto SAH(const SAHCalculateInfo & info) -> f32
{
    // Cost = 2 * T_AABB +
    //       ( A(S_l) / A(S) ) * N(S_l) * T_tri +
    //       ( A(S_r) / A(S) ) * N(S_r) * T_tri
    return 
        2.0f * info.ray_aabb_test_cost +
        (info.left_aabb_area / info.parent_aabb_area) * info.left_primitive_count * info.ray_tri_test_cost + 
        (info.right_aabb_area / info.parent_aabb_area) * info.right_primitive_count * info.ray_tri_test_cost;
}