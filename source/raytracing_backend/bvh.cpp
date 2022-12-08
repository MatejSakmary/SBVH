#include "bvh.hpp"

#include <algorithm>

auto BVH::SAH_greedy_best_split(const SAHGreedySplitInfo & info) -> SplitInfo
{
    // split algorithm
    f32 best_cost = info.primitive_aabbs.size() * info.ray_primitive_cost * info.node.bounding_box.get_area();
    Axis best_axis = Axis::LAST;
    i32 best_event = -1;

    auto & primitive_aabbs = info.primitive_aabbs;
    for(u32 axis = Axis::X; axis < Axis::LAST; axis++)
    {
        auto compare_op = [axis](const PrimitiveAABB & first, const PrimitiveAABB & second) -> bool
        {
            auto centroid_first = first.aabb.get_axis_centroid(static_cast<Axis>(axis));
            auto centroid_second = second.aabb.get_axis_centroid(static_cast<Axis>(axis));
            return centroid_first < centroid_second;
        };
        std::sort(primitive_aabbs.begin(), primitive_aabbs.end(), compare_op);

        // left sweep
        std::vector<f32> left_sweep_areas(primitive_aabbs.size(), 0.0);
        AABB left_sweep_aabb;
        for(size_t i = 0; i < primitive_aabbs.size(); i++)
        {
            left_sweep_aabb.expand_bounds(primitive_aabbs.at(i).aabb);
            left_sweep_areas.at(i) = left_sweep_aabb.get_area();
        }

        // right sweep
        AABB right_sweep_aabb;
        for(i32 i = primitive_aabbs.size() - 1; i >= 0 ; i--)
        {
            right_sweep_aabb.expand_bounds(primitive_aabbs.at(i).aabb);
            f32 cost = SAH({
                .left_primitive_count = static_cast<f32>(i),
                .right_primitive_count = primitive_aabbs.size() - static_cast<f32>(i),
                .left_aabb_area = left_sweep_areas.at(i),
                .right_aabb_area = right_sweep_aabb.get_area(),
                .parent_aabb_area = info.node.bounding_box.get_area(),
                .ray_aabb_test_cost = info.ray_aabb_test_cost,
                .ray_tri_test_cost = info.ray_primitive_cost
            });
            if(cost < best_cost)
            {
                best_cost = cost;
                best_event = i;
                best_axis = static_cast<Axis>(axis);
            }
        }
    }

    return {
        .axis = best_axis,
        .type = SplitType::OBJECT,
        .event = best_event,
        .cost = best_cost
    };
};

auto BVH::split_node(const SplitNodeInfo & info) -> void
{
    auto object_split = [&]()
    {
        auto compare_op = [&info](const PrimitiveAABB & first, const PrimitiveAABB & second) -> bool
        {
            auto centroid_first = first.aabb.get_axis_centroid(static_cast<Axis>(info.split.axis));
            auto centroid_second = second.aabb.get_axis_centroid(static_cast<Axis>(info.split.axis));
            return centroid_first < centroid_second;
        };
        std::sort(info.primitive_aabbs.begin(), info.primitive_aabbs.end(), compare_op);

        auto & left_child = bvh_nodes.emplace_back();
        info.node.left_index = bvh_nodes.size() - 1;
        for(int i = 0; i < info.split.event; i++)
        {
            left_child.bounding_box.expand_bounds(info.primitive_aabbs.at(i).aabb);
        }

        auto & right_child = bvh_nodes.emplace_back();
        info.node.right_index = bvh_nodes.size() - 1;
        for(int i = info.split.event; i < info.primitive_aabbs.size(); i++)
        {
            right_child.bounding_box.expand_bounds(info.primitive_aabbs.at(i).aabb);
        }
    };

    switch(info.split.type)
    {
        case SplitType::OBJECT:
        {
            object_split();
            return;
        }
        case SplitType::SPATIAL:
        {
            throw std::runtime_error("[BVH::split_node()] Spatial split not implemented yet");
            return;
        }
    }
}

auto BVH::construct_bvh_from_data(const std::vector<Triangle> & primitives) -> void
{
    // Generate vector of Primitive AABBs from the vector of primitives
    std::vector<PrimitiveAABB> primitive_aabbs;
    primitive_aabbs.reserve(primitives.size());
    auto primitives_it = primitives.begin();
    std::generate_n(std::back_inserter(primitive_aabbs), primitives.size(), [&]
    {
        return PrimitiveAABB
        {
            .aabb = AABB(*primitives_it),
            .primitive = &(*(primitives_it++))
        };
    });

    // Calculate the AABB of the scene -> stored in root node
    const u32 root_node_idx = 0;
    bvh_nodes.emplace_back();
    std::for_each(primitive_aabbs.begin(), primitive_aabbs.end(), [&](const PrimitiveAABB & aabb)
        {bvh_nodes.at(root_node_idx).bounding_box.expand_bounds(aabb.aabb);});

    SplitInfo sah_split = SAH_greedy_best_split({
        .ray_primitive_cost = 2.0f,
        .ray_aabb_test_cost = 3.0f,
        .node = bvh_nodes.back(),
        .primitive_aabbs = primitive_aabbs
    });

    DEBUG_VAR_OUT(sah_split.axis);
    DEBUG_VAR_OUT(sah_split.cost);
    DEBUG_VAR_OUT(sah_split.event);

    split_node({
        .split = sah_split,
        .primitive_aabbs = primitive_aabbs,
        .node = bvh_nodes.back()
    });
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