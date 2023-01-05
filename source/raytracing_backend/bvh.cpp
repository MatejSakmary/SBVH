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
            if(info.triangle[i][info.splitting_axis] > info.left_plane_axis_coord)
            {
                auto clipped_vertex = component_wise_min(info.triangle[i], info.parent_aabb.max_bounds);
                clipped_vertex = component_wise_max(clipped_vertex, info.parent_aabb.min_bounds);
                info.left_aabb.expand_bounds(clipped_vertex);
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
            auto intersect_vertex = glm::mix(left_vertex, right_vertex, dist_vertex_plane / dist_vertex_vertex);

            // clip the vertex to the parent bounding box -> this is needed because we may be splitting a primitive
            // which was already split before and so the parent bounding box does not fully contain the primitive.
            // This may result in the intersect_vertex being outside of the parent bounding box
            intersect_vertex = component_wise_min(intersect_vertex, info.parent_aabb.max_bounds);
            intersect_vertex = component_wise_max(intersect_vertex, info.parent_aabb.min_bounds);

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
    
    // project references into bins
    for(const auto & primitive_aabb : info.primitive_aabbs)
    {
        const auto & primitive = *primitive_aabb.primitive;

        i32vec3 start_bin_idx = glm::trunc(((primitive_aabb.aabb.min_bounds) - parent_aabb.min_bounds) / bin_size);
        i32vec3 end_bin_idx = glm::trunc(((primitive_aabb.aabb.max_bounds) - parent_aabb.min_bounds) / bin_size);
        // start_bin_idx and end_bin_idx should be value in the range [0, bin_count - 1]
        start_bin_idx = glm::clamp(start_bin_idx, i32vec3(0), i32vec3(info.bin_count - 1));
        end_bin_idx = glm::clamp(end_bin_idx, i32vec3(0), i32vec3(info.bin_count - 1));

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
            for(i32 bin_idx = i32(start_bin_idx[axis]) - 1; bin_idx <= end_bin_idx[axis]; bin_idx++)
            {
                // clip the primitive by the right bin border and expand the bins aabb
                project_primitive_into_bin({
                    .triangle = primitive,
                    .splitting_axis = static_cast<Axis>(axis),
                    .left_plane_axis_coord = parent_aabb.min_bounds[axis] + bin_size[axis] * glm::max(f32(bin_idx), -0.01f),
                    .right_plane_axis_coord = parent_aabb.min_bounds[axis] + bin_size[axis] * f32(bin_idx + 1u),
                    .parent_aabb = parent_aabb,
                    // there are some cases we have to take care of
                    //    - by the way the algorithm is designed we have to run for start_bin_idx - 1 which has no aabb 
                    //      left-of the splitting plane - we pass the aabb right-of splitting plane twice (so as left_aabb and right_aabb)
                    //    - when running for the right-most aabb (which is passed as left_aabb in this case) there are no more right
                    //      aabbs to pass into the right_aabb parameters - we pass the aabb left-of splitting plane twice (so as left_aabb and right_aabb)
                    // in both cases the run will not add anything to the right/left aabb respectively so this is ok
                    .left_aabb = bin_aabbs.at(axis).at(glm::max(bin_idx, start_bin_idx[axis])),
                    .right_aabb = bin_aabbs.at(axis).at(glm::min(bin_idx + 1, end_bin_idx[axis]))
                });
            }
        }
    }

    AABB left_bounding_box;
    AABB right_bounding_box;
    f32 best_cost = INFINITY;
    Axis best_axis = Axis::LAST;
    i32 best_event = -1;

    for(i32 axis = Axis::X; axis < Axis::LAST; axis++)
    {
        std::vector<AABB> left_sweep_aabbs(info.bin_count);
        std::vector<u32> left_sweep_bin_primitives(info.bin_count, 0);
        AABB left_sweep_aabb;
        for(i32 bin = 0; bin < info.bin_count; bin++)
        {
            left_sweep_aabb.expand_bounds(bin_aabbs.at(axis).at(bin));
            left_sweep_aabbs.at(bin) = left_sweep_aabb;
            left_sweep_bin_primitives.at(bin) =
                left_sweep_bin_primitives.at(glm::max(0, bin - 1)) +
                bin_counters.at(axis).at(bin).at(START_BIN_IDX);
        }

        AABB right_sweep_aabb;
        f32 right_sweep_aabb_area = 0;
        u32 right_sweep_bin_primitives = 0;
        // The stopping condition is not >= 0 because that would just result in the left bin being empty
        // and the right bin being the entire child. This, however is the same as the left bin being 
        // the entire child and the right bin being empty which is the case in the first iteration
        // of the loop -> the right_sweep_area is 0 and left_sweep_area is the area of the entire node
        for(i32 bin = i32(info.bin_count - 1); bin > 0; bin--)
        {
            f32 cost = SAH({
                .left_primitive_count = left_sweep_bin_primitives.at(bin),
                .right_primitive_count = right_sweep_bin_primitives,
                .left_aabb_area = left_sweep_aabbs.at(bin).get_area(),
                .right_aabb_area = right_sweep_aabb_area,
                .parent_aabb_area = parent_node.bounding_box.get_area(),
                .ray_aabb_test_cost = info.ray_aabb_test_cost,
                .ray_tri_test_cost = info.ray_primitive_cost
            });

            if(cost < best_cost)
            {
                best_cost = cost;
                best_event = bin;
                best_axis = static_cast<Axis>(axis);
                left_bounding_box = left_sweep_aabbs.at(bin);
                right_bounding_box = right_sweep_aabb;
            }

            right_sweep_aabb.expand_bounds(bin_aabbs.at(axis).at(bin));
            right_sweep_aabb_area = right_sweep_aabb.get_area();
            right_sweep_bin_primitives += bin_counters.at(axis).at(bin).at(END_BIN_IDX);
        }
    }

    return SplitInfo {
        .axis = best_axis,
        .type = SplitType::SPATIAL,
        .event = best_event,
        .cost = best_cost,
        .left_bounding_box = left_bounding_box,
        .right_bounding_box = right_bounding_box
    };
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
                .left_primitive_count = i,
                .right_primitive_count = primitive_aabbs.size() - i,
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

auto BVH::split_node(const SplitNodeInfo & info) -> SplitPrimitives
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

    auto spatial_split = [&]()
    {
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

            // Assume the list is sorted (using the splitting axis) - this is done in the object_split() function
            std::vector<PrimitiveAABB> left_primitive_aabbs;
            std::vector<PrimitiveAABB> right_primitive_aabbs;
            left_primitive_aabbs.reserve(info.split.event);
            right_primitive_aabbs.reserve(info.primitive_aabbs.size() - info.split.event);
            std::copy(info.primitive_aabbs.begin(), info.primitive_aabbs.begin() + info.split.event, std::back_inserter(left_primitive_aabbs));
            std::copy(info.primitive_aabbs.begin() + info.split.event, info.primitive_aabbs.end(), std::back_inserter(right_primitive_aabbs));
            return {left_primitive_aabbs, right_primitive_aabbs};
        }
        case SplitType::SPATIAL:
        {
            throw std::runtime_error("[BVH::split_node()] Spatial split not implemented yet");
            return {};
        }
    }
    return {};
}

auto BVH::construct_bvh_from_data(const std::vector<Triangle> & primitives, const ConstructBVHInfo & info) -> void
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

    using ProcessNode = std::pair<u32, std::vector<PrimitiveAABB>>; 
    std::queue<ProcessNode> nodes;
    nodes.push({root_node_idx, primitive_aabbs});

    while(!nodes.empty())
    {
        auto & [node_idx, primitive_aabbs] = nodes.front();

        SplitInfo best_split = SAH_greedy_best_split({
            .ray_primitive_cost = info.ray_primitive_intersection_cost,
            .ray_aabb_test_cost = info.ray_aabb_intersection_cost,
            .node_idx = node_idx,
            .primitive_aabbs = primitive_aabbs
        });

        // try spatial split only if the boxes intersect and their intersection is big enough 
        // compared to the AABB of the of the scene -> this allows spatial splits only in the 
        // upper levels of the BVH where spatial splits are the most efficient
        if(do_aabbs_intersect(best_split.left_bounding_box, best_split.right_bounding_box))
        {
            f32 lambda = get_intersection_aabb(
                best_split.left_bounding_box,
                best_split.right_bounding_box).get_area();

            // check for the intersection size
            if(lambda / bvh_nodes.at(root_node_idx).bounding_box.get_area() > info.spatial_alpha)
            {
                SplitInfo spatial_split = spatial_best_split({
                    .ray_primitive_cost = info.ray_primitive_intersection_cost,
                    .ray_aabb_test_cost = info.ray_aabb_intersection_cost,
                    .bin_count = info.spatial_bin_count,
                    .node_idx = node_idx,
                    .primitive_aabbs = primitive_aabbs
                });
                if(spatial_split.cost < best_split.cost) { best_split = spatial_split; }
            } 
        }

        if(best_split.event == -1)
        {
            nodes.pop();
            continue;
        }

        auto [left_primitive_aabbs, right_primitive_aabbs] = split_node({
            .split = best_split,
            .primitive_aabbs = primitive_aabbs,
            .node_idx = node_idx
        });

        if(left_primitive_aabbs.size() > 1) { nodes.emplace(bvh_nodes.at(node_idx).left_index, std::move(left_primitive_aabbs));}
        if(right_primitive_aabbs.size() > 1){ nodes.emplace(bvh_nodes.at(node_idx).right_index, std::move(right_primitive_aabbs));}
        nodes.pop();
    }
}

auto BVH::get_bvh_visualization_data() const -> std::vector<AABBGeometryInfo>
{
    std::vector<AABBGeometryInfo> info;
    info.reserve(bvh_nodes.size());
    if(bvh_nodes.empty()) {return info;}

    const auto & root_node = bvh_nodes.at(0);
    using Node = std::pair<u32, const BVHNode &>;  

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