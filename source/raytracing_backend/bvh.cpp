#include "bvh.hpp"

#include <algorithm>

AABB::AABB() : left_bottom_front(INFINITY), right_top_back(-INFINITY) {}

AABB::AABB(const f32vec3 & left_bottom_front, const f32vec3 & right_top_back) :
    left_bottom_front{left_bottom_front}, right_top_back{right_top_back} {}

auto AABB::expand_bounds(const f32vec3 & vertex) -> void
{
    left_bottom_front = component_wise_min(left_bottom_front, vertex);
    right_top_back = component_wise_max(right_top_back, vertex);
};

auto AABB::expand_bounds(const Triangle & triangle) -> void
{
    left_bottom_front = component_wise_min(left_bottom_front, triangle.v0);
    left_bottom_front = component_wise_min(left_bottom_front, triangle.v1);
    left_bottom_front = component_wise_min(left_bottom_front, triangle.v2);
    right_top_back = component_wise_max(right_top_back, triangle.v0);
    right_top_back = component_wise_max(right_top_back, triangle.v1);
    right_top_back = component_wise_max(right_top_back, triangle.v2);
}

auto AABB::get_area() const -> f32
{
    f32vec3 sizes = right_top_back - left_bottom_front;
    return { 2 * sizes.x * sizes.y + 2 * sizes.y * sizes.z + 2 * sizes.z * sizes.x };
}

auto BVH::construct_bvh_from_data(std::vector<Triangle> & primitives) -> void
{
    // Calculate the AABB of the scene
    const float leaf_create_cost = 2.0f;
    auto & node = bvh_nodes.emplace_back();
    for(const auto & primitive : primitives)
    {
        node.bounding_box.expand_bounds(primitive);
    }

    // split algorithm
    f32 best_cost = primitives.size() * leaf_create_cost * node.bounding_box.get_area();
    Axis best_axis = Axis::LAST;
    i32 best_event = -1;

    for(u32 axis = Axis::X; axis < Axis::LAST; axis++)
    {
        auto compare_op = [axis](const Triangle & first, const Triangle & second) -> bool
        {
            auto centroid_first = first.get_aabb_axis_centroid(static_cast<Axis>(axis));
            auto centroid_second = second.get_aabb_axis_centroid(static_cast<Axis>(axis));
            return centroid_first < centroid_second;
        };
        std::sort(primitives.begin(), primitives.end(), compare_op);

        // left sweep
        std::vector<f32> left_sweep_areas(primitives.size(), 0.0);
        AABB left_sweep_aabb;
        for(size_t i = 0; i < primitives.size(); i++)
        {
            left_sweep_aabb.expand_bounds(primitives.at(i));
            left_sweep_areas.at(i) = left_sweep_aabb.get_area();
        }

        // right sweep
        AABB right_sweep_aabb;
        for(i32 i = primitives.size() - 1; i >= 0 ; i--)
        {
            right_sweep_aabb.expand_bounds(primitives.at(i));
            f32 cost = SAH({
                .left_primitive_count = static_cast<f32>(i),
                .right_primitive_count = primitives.size() - static_cast<f32>(i),
                .left_aabb_area = left_sweep_areas.at(i),
                .right_aabb_area = right_sweep_aabb.get_area(),
                .parent_aabb_area = node.bounding_box.get_area(),
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
    node.left_index = bvh_nodes.size() - 1;
    for(int i = 0; i < best_event; i++)
    {
        left_child.bounding_box.expand_bounds(primitives.at(i));
    }

    auto & right_child = bvh_nodes.emplace_back();
    node.right_index = bvh_nodes.size() - 1;
    for(int i = best_event; i < primitives.size(); i++)
    {
        right_child.bounding_box.expand_bounds(primitives.at(i));
    }
}

auto BVH::get_bvh_visualization_data() const -> std::vector<AABBGeometryInfo>
{
    std::vector<AABBGeometryInfo> info;
    info.reserve(bvh_nodes.size());

    f32 depth = 0.0f;
    for(const auto & node : bvh_nodes)
    {
        const auto & aabb = node.bounding_box;
        info.emplace_back(AABBGeometryInfo{
            .position = daxa_vec3_from_glm(aabb.left_bottom_front),
            .scale = daxa_vec3_from_glm(aabb.right_top_back - aabb.left_bottom_front),
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
    { 
        2.0f * info.ray_aabb_test_cost +
        (info.left_aabb_area / info.parent_aabb_area) * info.left_primitive_count * info.ray_tri_test_cost + 
        (info.right_aabb_area / info.parent_aabb_area) * info.right_primitive_count * info.ray_tri_test_cost
    };
}