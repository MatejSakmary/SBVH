#include "bvh.hpp"

#include <algorithm>
#include <queue>

auto BVH::SAH_greedy_best_split(const SAHGreedySplitInfo & info) -> SplitInfo
{
    // split algorithm
    f32 best_cost = info.primitive_aabbs.size() * info.ray_primitive_cost * bvh_nodes.at(info.node_idx).bounding_box.get_area();
    Axis best_axis = Axis::LAST;
    i32 best_event = -1;
    AABB left_bounding_box;
    AABB right_bounding_box;

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
        std::vector<AABB> left_sweep_aabbs(primitive_aabbs.size());
        AABB left_sweep_aabb;
        for(size_t i = 0; i < primitive_aabbs.size(); i++)
        {
            left_sweep_aabb.expand_bounds(primitive_aabbs.at(i).aabb);
            left_sweep_aabbs.at(i) = left_sweep_aabb;
        }

        // right sweep
        AABB right_sweep_aabb;
        for(i32 i = primitive_aabbs.size() - 1; i >= 0 ; i--)
        {
            right_sweep_aabb.expand_bounds(primitive_aabbs.at(i).aabb);
            f32 cost = SAH({
                .left_primitive_count = static_cast<f32>(i),
                .right_primitive_count = primitive_aabbs.size() - static_cast<f32>(i),
                // TODO(msakmary) fix this
                .left_aabb_area = i == 0 ? 0 : left_sweep_aabbs.at(i - 1).get_area(),
                .right_aabb_area = right_sweep_aabb.get_area(),
                .parent_aabb_area = bvh_nodes.at(info.node_idx).bounding_box.get_area(),
                .ray_aabb_test_cost = info.ray_aabb_test_cost,
                .ray_tri_test_cost = info.ray_primitive_cost
            });
            if(cost < best_cost)
            {
                best_cost = cost;
                best_event = i;
                best_axis = static_cast<Axis>(axis);
                // TODO(msakmary) FIX THIS!!
                left_bounding_box = left_sweep_aabbs.at(i - 1);
                right_bounding_box = right_sweep_aabb;
            }
        }
    }

    return {
        .axis = best_axis,
        .type = SplitType::OBJECT,
        .event = best_event,
        .cost = best_cost,
        .left_bounding_box = left_bounding_box,
        .right_bounding_box = right_bounding_box
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
        left_child.bounding_box = info.split.left_bounding_box;
        bvh_nodes.at(info.node_idx).left_index = bvh_nodes.size() - 1;

        auto & right_child = bvh_nodes.emplace_back();
        right_child.bounding_box = info.split.right_bounding_box;
        bvh_nodes.at(info.node_idx).right_index = bvh_nodes.size() - 1;
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

    using ToProcessNode = std::pair<u32, std::vector<PrimitiveAABB>>; 
    std::queue<ToProcessNode> nodes;
    nodes.push({root_node_idx, primitive_aabbs});

    while(!nodes.empty())
    {
        auto & [node_idx, primitive_aabbs] = nodes.front();
        SplitInfo sah_split = SAH_greedy_best_split({
            .ray_primitive_cost = 2.0f,
            .ray_aabb_test_cost = 3.0f,
            .node_idx = node_idx,
            .primitive_aabbs = primitive_aabbs
        });

        DEBUG_VAR_OUT(sah_split.axis);
        DEBUG_VAR_OUT(sah_split.cost);
        DEBUG_VAR_OUT(sah_split.event);
        DEBUG_VAR_OUT(primitive_aabbs.size());

        if(sah_split.event == -1)
        {
            nodes.pop();
            continue;
        }

        split_node({
            .split = sah_split,
            .primitive_aabbs = primitive_aabbs,
            .node_idx = node_idx
        });

        // Assume the list is sorted (using the splitting axis) in the split_node()
        std::vector<PrimitiveAABB> left_primitive_aabbs;
        std::vector<PrimitiveAABB> right_primitive_aabbs;
        left_primitive_aabbs.reserve(sah_split.event);
        right_primitive_aabbs.reserve(sah_split.event);
        std::copy(primitive_aabbs.begin(), primitive_aabbs.begin() + sah_split.event, std::back_inserter(left_primitive_aabbs));
        std::copy(primitive_aabbs.begin() + sah_split.event, primitive_aabbs.end(), std::back_inserter(right_primitive_aabbs));

        if(left_primitive_aabbs.size() > 1) nodes.emplace(bvh_nodes.at(node_idx).left_index, std::move(left_primitive_aabbs));
        if(right_primitive_aabbs.size() > 1) nodes.emplace(bvh_nodes.at(node_idx).right_index, std::move(right_primitive_aabbs));
        nodes.pop();
    }
}

auto BVH::get_bvh_visualization_data() const -> std::vector<AABBGeometryInfo>
{
    std::vector<AABBGeometryInfo> info;
    info.reserve(bvh_nodes.size());

    auto & root_node = bvh_nodes.at(0);
    using Node = std::pair<f32, const BVHNode &>;  
    std::queue<Node> que;

    que.push({0.0f, root_node});
    while(!que.empty())
    {
        const auto & [depth, node] = que.front();
        const auto & aabb = node.bounding_box;
        info.emplace_back(AABBGeometryInfo{
            .position = daxa_vec3_from_glm(aabb.min_bounds),
            .scale = daxa_vec3_from_glm(aabb.max_bounds - aabb.min_bounds),
            .depth = static_cast<daxa::f32>(depth)
        });

        
        if(node.left_index != 0) que.push({depth + 1.0f, bvh_nodes.at(node.left_index)});
        if(node.right_index != 0) que.push({depth + 1.0f, bvh_nodes.at(node.right_index)});
        que.pop();
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