#include "bvh.hpp"

#include <algorithm>
#include <cassert>
#include <glm/common.hpp>
#include <glm/vector_relational.hpp>
#include <queue>
#include <array>

auto BVH::project_primitive_into_bin(const ProjectPrimitiveInfo & info) -> void
{
    // We are sure that all of the points are not left of the border
    // -> there is atleast one point and at most two points on the right side of the border

    // store the indices of the vertices which are to the right of the border and which are to the left
    std::vector<i32> indices_right_of;
    std::vector<i32> indices_left_of;
    indices_right_of.reserve(2);
    indices_left_of.reserve(2);

    // sort the indices
    for(int i = 0; i < 3; i++)
    {
        if(info.triangle[i][info.splitting_axis] <= info.right_plane_axis_coord)
        {
            // add the left vertex to the left aabb
            if(info.triangle[i][info.splitting_axis] >= info.left_plane_axis_coord)
            {
                info.left_aabb.expand_bounds(info.triangle[i]);
            }
            indices_left_of.push_back(i);
        } else {
            indices_right_of.push_back(i);
        }
    }

    // all three triangle vertices were to the left of the right splitting plane
    // this can only happen for the last bin exit early as there is no more work
    if(indices_left_of.size() == 3) { return; };

    // take all of the vertices on the left of the border and find intersection
    // with the splitting plane using linear interpolation: 
    //      The ratio between distances:
    //          t = dist(left_vert.axis,split_plane.axis) / dist(left_vert.axis,right_vert.axis)
    //          where .axis is the coordinate in the splitting axis
    //      gives the interpolation factor t which is then used to interpolate between 
    //      left_vert and right_vert (left_vert + t * right_vert) giving us the intersection point
    
    // We loop through all the points on the left side of the border (which are either one or two)
    // and calculate all the edges with the points on the right side of border 
    // (which are either two or one respectively)
    // This covers both scenarios 1) v0 v1 | v2 and v0 | v1 v2 since in the first case the inner
    // forloop will just execute once and in the second the outer one will only execute once
    for(const auto left_index : indices_left_of)
    {
        const auto & left_vertex = info.triangle[left_index];

        // dist_vertex_plane  (vertex_plane)  -> distance(left_vert.axis, split_plane.axis)
        // dist_vertex_vertex (vertex vertex) -> distance(left_vert.axis, right_vert.axis)
        f32 dist_vertex_plane = info.right_plane_axis_coord - left_vertex[info.splitting_axis];
        for(const auto right_index : indices_right_of)
        {
            const auto & right_vertex = info.triangle[right_index];
            f32 dist_vertex_vertex = right_vertex[info.splitting_axis] - left_vertex[info.splitting_axis];
            // this may result in bin aabb to be way too big -> fixed before computing SAH
            const auto intersect_vertex = glm::mix(left_vertex, right_vertex, dist_vertex_plane / dist_vertex_vertex);

            // expand left and right aabb since the point lies exactly on the border
            info.left_aabb.expand_bounds(intersect_vertex);
            info.right_aabb.expand_bounds(intersect_vertex);
        }
    }
}

auto BVH::spatial_best_split(const SpatialSplitInfo & info) -> SplitInfo
{
    const auto & parent_node = bvh_nodes[info.node_idx];
    const auto & parent_aabb = parent_node.bounding_box;

    const f32vec3 node_size = parent_aabb.max_bounds - parent_aabb.min_bounds;
    const f32vec3 bin_size = node_size / f32(info.bin_count);

    // each bin counter consist of two separate counters for start and end indices
    using BinCounters = std::vector<std::array<u32,2>>;

    // first dimensionn is the 3 element array -> the axis (0 = X, 1 = Y, 2 = Z)
    // second dimenstion is the vector dimension which is the bin index 
    // third dimension is the 2 element array -> start/end array (0 - start array, 1 - end array)
    const u32 START_BIN_IDX = 0;
    const u32 END_BIN_IDX = 1;
    std::array<BinCounters,3> bin_counters{
        BinCounters(info.bin_count, std::array<u32, 2>({0u, 0u})),
        BinCounters(info.bin_count, std::array<u32, 2>({0u, 0u})),
        BinCounters(info.bin_count, std::array<u32, 2>({0u, 0u}))};

    // first dimension is the axis in which we are splitting
    // second dimension is the bin index
    std::array<std::vector<AABB>, 3> bin_aabbs {
        std::vector<AABB>(info.bin_count),
        std::vector<AABB>(info.bin_count),
        std::vector<AABB>(info.bin_count)};
    
    // used to offset max and min bounds of the primitive aabb in order to get correct start and end indices
    // in case parent aabb perfectly alligns with the primitive aabb
    const f32vec3 OFFSET = f32vec3(0.001f) * bin_size;
    // project references into bins
    for(const auto & primitive_aabb : info.primitive_aabbs)
    {
        const auto & primitive = *primitive_aabb.primitive;

        u32vec3 start_bin_idx = glm::trunc(((primitive_aabb.aabb.min_bounds + OFFSET) - parent_aabb.min_bounds) / bin_size);
        u32vec3 end_bin_idx = glm::trunc(((primitive_aabb.aabb.max_bounds - OFFSET) - parent_aabb.min_bounds) / bin_size);
        // start_bin_idx and end_bin_idx should be value in the range [0, bin_count - 1]
        assert(glm::all(glm::greaterThanEqual(start_bin_idx, u32vec3(0u))) &&
               glm::all(glm::lessThan(start_bin_idx, u32vec3(info.bin_count))));

        assert(glm::all(glm::greaterThanEqual(end_bin_idx, u32vec3(0u))) &&
               glm::all(glm::lessThan(end_bin_idx, u32vec3(info.bin_count))));

        for(i32 axis = Axis::X; axis < Axis::LAST; axis++)
        {
            // start_bin_idx[axis] - returns the bin index which is the first (from the left) which the 
            // primitive intersects in the selected splitting axis
            // [axis][bin_idx][start/end]
            bin_counters.at(axis).at(start_bin_idx[axis]).at(START_BIN_IDX) += 1;
            bin_counters.at(axis).at(end_bin_idx[axis]).at(END_BIN_IDX) += 1;

            // entire aabb lies in the bin just add it directly
            if(start_bin_idx[axis] == end_bin_idx[axis])
            {
                bin_aabbs.at(axis).at(start_bin_idx[axis]).expand_bounds(primitive_aabb.aabb);
                continue;
            }
            // project the primitive into separate bins
            for(u32 bin_idx = start_bin_idx[axis]; bin_idx <= end_bin_idx[axis]; bin_idx++)
            {
                // clip the primitive by the right bin border and expand the bins aabb
                project_primitive_into_bin({
                    .triangle = primitive,
                    .splitting_axis = static_cast<Axis>(axis),
                    .left_plane_axis_coord = parent_aabb.min_bounds[axis] + bin_size[axis] * f32(bin_idx),
                    .right_plane_axis_coord = parent_aabb.min_bounds[axis] + bin_size[axis] * f32(bin_idx + 1u),
                    .left_aabb = bin_aabbs.at(axis).at(bin_idx),
                    // when we are processing the right-most bin there are no further right bins
                    //    - we pass the left bin twice here which fixes this problem with no further side effects
                    .right_aabb = bin_idx < end_bin_idx[axis] ?
                        bin_aabbs.at(axis).at(bin_idx + 1) :
                        bin_aabbs.at(axis).at(bin_idx)
                });
            }
        }
    }
    
    for(i32 axis = Axis::X; axis < Axis::LAST; axis++)
    {
        for(const auto & bin_aabb : bin_aabbs.at(axis))
        {
            auto & new_node = bvh_nodes.emplace_back();
            new_node.bounding_box = bin_aabb;
            new_node.left_index = axis;
        }
    }
    // clip the bins' AABBs to the maximum possible size (this needs to be done because there were some checks
    // we omitted from the project_primitive_into_bin function which may result in AABBs which extend over the 
    // maximum possible bin AABB)
    for(i32 axis = Axis::X; axis < Axis::LAST; axis++)
    {
        // TODO(msakmary) CONTINUE HERE!
    }

    return {};
}

auto BVH::SAH_greedy_best_split(const SAHGreedySplitInfo & info) -> SplitInfo
{
    // split algorithm
    f32 best_cost = f32(info.primitive_aabbs.size()) * info.ray_primitive_cost * bvh_nodes.at(info.node_idx).bounding_box.get_area();
    Axis best_axis = Axis::LAST;
    i32 best_event = -1;
    AABB left_bounding_box;
    AABB right_bounding_box;

    auto & primitive_aabbs = info.primitive_aabbs;
    for(i32 axis = Axis::X; axis < Axis::LAST; axis++)
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
        for(i32 i = i32(primitive_aabbs.size() - 1); i >= 0 ; i--)
        {
            right_sweep_aabb.expand_bounds(primitive_aabbs.at(i).aabb);
            f32 cost = SAH({
                .left_primitive_count = static_cast<f32>(i),
                .right_primitive_count = static_cast<f32>(primitive_aabbs.size()) - static_cast<f32>(i),
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
        bvh_nodes.at(info.node_idx).left_index = i32(bvh_nodes.size() - 1);

        auto & right_child = bvh_nodes.emplace_back();
        right_child.bounding_box = info.split.right_bounding_box;
        bvh_nodes.at(info.node_idx).right_index = i32(bvh_nodes.size() - 1);
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

    const f32 RAY_PRIMITVE_INTERSECTION_COST = 2.0f;
    const f32 RAY_AABB_INTERSECTION_COST = 3.0f;



    while(!nodes.empty())
    {
        auto & [node_idx, primitive_aabbs] = nodes.front();

        SplitInfo spatial_split = spatial_best_split({
            .ray_primitive_cost = RAY_PRIMITVE_INTERSECTION_COST,
            .ray_aabb_test_cost = RAY_AABB_INTERSECTION_COST,
            .bin_count = 4,
            .node_idx = node_idx,
            .primitive_aabbs = primitive_aabbs
        });
        return;

        SplitInfo sah_split = SAH_greedy_best_split({
            .ray_primitive_cost = RAY_PRIMITVE_INTERSECTION_COST,
            .ray_aabb_test_cost = RAY_AABB_INTERSECTION_COST,
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

        if(left_primitive_aabbs.size() > 1) { nodes.emplace(bvh_nodes.at(node_idx).left_index, std::move(left_primitive_aabbs));}
        if(right_primitive_aabbs.size() > 1){ nodes.emplace(bvh_nodes.at(node_idx).right_index, std::move(right_primitive_aabbs));}
        nodes.pop();
    }
}

auto BVH::get_bvh_visualization_data() const -> std::vector<AABBGeometryInfo>
{


    std::vector<AABBGeometryInfo> info;
    info.reserve(bvh_nodes.size());

    const auto & root_node = bvh_nodes.at(0);
    using Node = std::pair<u32, const BVHNode &>;  
    // SPATIAL SPLIT DEBUG CODE
    for(const auto & bvh_node : bvh_nodes)
    {
        info.emplace_back(AABBGeometryInfo{
            .position = daxa_vec3_from_glm(bvh_node.bounding_box.min_bounds),
            .scale = daxa_vec3_from_glm(bvh_node.bounding_box.max_bounds - bvh_node.bounding_box.min_bounds),
            .depth = static_cast<daxa::u32>(bvh_node.left_index)
        });
    }
    return info;

    // LEGIT CODE
    std::queue<Node> que;
    que.push({0u, root_node});
    while(!que.empty())
    {
        const auto & [depth, node] = que.front();
        const auto & aabb = node.bounding_box;
        info.emplace_back(AABBGeometryInfo{
            .position = daxa_vec3_from_glm(aabb.min_bounds),
            .scale = daxa_vec3_from_glm(aabb.max_bounds - aabb.min_bounds),
            .depth = static_cast<daxa::u32>(depth)
        });
        
        if(node.left_index != 0) { que.push({depth + 1u, bvh_nodes.at(node.left_index)});}
        if(node.right_index != 0){ que.push({depth + 1u, bvh_nodes.at(node.right_index)});}
        que.pop();
    }
    return info;
}

auto SAH(const SAHCalculateInfo & info) -> f32
{
    // Cost = 2 * T_AABB +
    //       ( A(S_l) / A(S) ) * N(S_l) * T_tri +
    //       ( A(S_r) / A(S) ) * N(S_r) * T_tri
    const float child_aabb_count = 2.0f;
    return 
        child_aabb_count * info.ray_aabb_test_cost +
        (info.left_aabb_area / info.parent_aabb_area) * info.left_primitive_count * info.ray_tri_test_cost + 
        (info.right_aabb_area / info.parent_aabb_area) * info.right_primitive_count * info.ray_tri_test_cost;
}