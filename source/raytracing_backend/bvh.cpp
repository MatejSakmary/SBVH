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

auto BVH::spatial_best_split(const SpatialSplitInfo & info) -> BestSplitInfo
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
            for(i32 bin_idx = i32(start_bin_idx[axis]) - 1; bin_idx <= end_bin_idx[axis]; bin_idx++)
            {
                // clip the primitive by the right bin border and expand the bins aabb

                // there are some cases we have to take care of
                //    - by the way the algorithm is designed we have to run for start_bin_idx - 1 which has no aabb 
                //      left-of the splitting plane - we pass the aabb right-of splitting plane twice (so as left_aabb and right_aabb)
                //    - when running for the right-most aabb (which is passed as left_aabb in this case) there are no more right
                //      aabbs to pass into the right_aabb parameters - we pass the aabb left-of splitting plane twice
                //      (so as left_aabb and right_aabb)
                // in both cases the run will not add anything to the right/left aabb respectively so this is ok
                project_primitive_into_bin({
                    .triangle = primitive,
                    .splitting_axis = static_cast<Axis>(axis),
                    .left_plane_axis_coord = parent_aabb.min_bounds[axis] + bin_size[axis] * glm::max(f32(bin_idx), -0.01f),
                    .right_plane_axis_coord = parent_aabb.min_bounds[axis] + bin_size[axis] * f32(bin_idx + 1u),
                    .parent_aabb = parent_aabb,
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
    f32 best_event = -1;

    for(i32 axis = Axis::X; axis < Axis::LAST; axis++)
    {
        std::vector<AABB> left_sweep_aabbs(info.bin_count);
        std::vector<u32> left_sweep_bin_primitives(info.bin_count, 0);
        AABB left_sweep_aabb;
        for(i32 bin = 0; bin < info.bin_count; bin++)
        {
            left_sweep_aabb.expand_bounds(bin_aabbs.at(axis).at(bin));
            left_sweep_aabbs.at(bin) = left_sweep_aabb;
            // summing the bin primtives from the left - if there is no bin to the left of this one 
            // (this bin is the first) add the number of start bins to yourself (thats what the max is for)
            left_sweep_bin_primitives.at(bin) =
                left_sweep_bin_primitives.at(glm::max(0, bin - 1)) +
                bin_counters.at(axis).at(bin).at(START_BIN_IDX);
        }

        AABB right_sweep_aabb;
        u32 right_sweep_bin_primitives = 0;
        for(i32 bin = i32(info.bin_count - 2); bin >= 0; bin--)
        {
            // expand bounds of the right sweep bin and increase the count *after* the cost is computed
            //   - this is because left sweep starts with with the 0th bin in the 0th element
            //     and thus ends in the entire node contained in the last element. So the right sweep than
            //     starts by having no primitives and empty AABB end ends by containing everything except
            //     the last bin to properly match the left sweep. Similarly to object split, we do not need
            //     to check the condition where right sweep contains the entire node as this would be duplicit.
            f32 cost = SAH({
                .left_primitive_count = left_sweep_bin_primitives.at(bin),
                .right_primitive_count = right_sweep_bin_primitives,
                .left_aabb_area = left_sweep_aabbs.at(bin).get_area(),
                .right_aabb_area = right_sweep_aabb.get_area(),
                .parent_aabb_area = parent_node.bounding_box.get_area(),
                .ray_aabb_test_cost = info.ray_aabb_test_cost,
                .ray_tri_test_cost = info.ray_primitive_cost
            });

            if(cost < best_cost && left_sweep_bin_primitives.at(bin) > 1 &&
               right_sweep_bin_primitives > 1 && bin_size[axis] > 0.001f)
            {
                best_cost = cost;
                best_event = parent_aabb.min_bounds[axis] + bin_size[axis] * (bin + 1);
                best_axis = static_cast<Axis>(axis);
                left_bounding_box = left_sweep_aabbs.at(bin);
                right_bounding_box = right_sweep_aabb;
            }

            right_sweep_aabb.expand_bounds(bin_aabbs.at(axis).at(bin));
            right_sweep_bin_primitives += bin_counters.at(axis).at(bin).at(END_BIN_IDX);
        }
    }

    return BestSplitInfo {
        .axis = best_axis,
        .type = SplitType::SPATIAL,
        .event = best_event,
        .cost = best_cost,
        .left_bounding_box = left_bounding_box,
        .right_bounding_box = right_bounding_box
    };
}

auto BVH::SAH_greedy_best_split(const SAHGreedySplitInfo & info) -> BestSplitInfo
{
    // split algorithm
    f32 best_cost = INFINITY;
    if(info.join_leaves)
    {
        best_cost = f32(info.primitive_aabbs.size()) * info.ray_primitive_cost * bvh_nodes.at(info.node_idx).bounding_box.get_area();
    }
    f32 bbarea = bvh_nodes.at(info.node_idx).bounding_box.get_area();
    assert(best_cost != 0.0f);
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
            if(centroid_first == centroid_second)
            {
                assert(first.aabb.get_area() == second.aabb.get_area());
                return first.aabb.get_area() < second.aabb.get_area();
            }
            return centroid_first < centroid_second;
        };
        std::sort(primitive_aabbs.begin(), primitive_aabbs.end(), compare_op);

        // left sweep
        std::vector<AABB> left_sweep_aabbs(primitive_aabbs.size());
        AABB left_sweep_aabb;
        for(size_t i = 0; i < primitive_aabbs.size(); i++)
        {
            left_sweep_aabbs.at(i) = left_sweep_aabb;
            left_sweep_aabb.expand_bounds(primitive_aabbs.at(i).aabb);
        }

        // right sweep
        AABB right_sweep_aabb;
        for(i32 i = i32(primitive_aabbs.size() - 1); i > 0 ; i--)
        {
            // expand bounds first since in the left sweep we expanded bounds *after* getting the AABB
            //   - this results in the left having empty AABB in the 0th element, and AABB containing 
            //     everything except the last primitive in the last element. This means we have to start
            //     the right loop by expanding the right AABB to contain the last primtive. This is guaranteed
            //     to check all the possible combinations except for the case where left/right would contain the
            //     entire scene  
            right_sweep_aabb.expand_bounds(primitive_aabbs.at(i).aabb);

            f32 cost = SAH({
                .left_primitive_count = static_cast<u32>(i),
                .right_primitive_count = static_cast<u32>(primitive_aabbs.size() - i),
                .left_aabb_area = left_sweep_aabbs.at(i).get_area(),
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
                left_bounding_box = left_sweep_aabbs.at(i);
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
            if(centroid_first == centroid_second)
            {
                assert(first.aabb.get_area() == second.aabb.get_area());
                return first.aabb.get_area() < second.aabb.get_area();
            }
            return centroid_first < centroid_second;
        };
        std::sort(info.primitive_aabbs.begin(), info.primitive_aabbs.end(), compare_op);

        auto & left_child = bvh_nodes.emplace_back();
        left_child.bounding_box = info.split.left_bounding_box;
#ifdef VISUALIZE_SPATIAL_SPLITS
        left_child.spatial = 0u;
#endif
        bvh_nodes.at(info.node_idx).left_index = i32(bvh_nodes.size() - 1);

        auto & right_child = bvh_nodes.emplace_back();
        right_child.bounding_box = info.split.right_bounding_box;
#ifdef VISUALIZE_SPATIAL_SPLITS
        right_child.spatial = 0u;
#endif
        bvh_nodes.at(info.node_idx).right_index = i32(bvh_nodes.size() - 1);
    };

    auto spatial_split = [&]() -> SplitPrimitives
    {
        std::vector<PrimitiveAABB> left_primitive_aabbs;
        std::vector<PrimitiveAABB> right_primitive_aabbs;
        std::vector<PrimitiveAABB> border_primitive_aabbs;
        f32 splitting_plane = std::get<f32>(info.split.event);
        const auto & parent_node = bvh_nodes.at(info.node_idx);

        AABB left_aabb;
        AABB right_aabb;
        for(const auto & primitive : info.primitive_aabbs)
        {
            if(!primitive.aabb.check_if_valid())
            {
                continue;
            }

            // The entire primitive is on the left side of the splitting plane
            if(primitive.aabb.max_bounds[info.split.axis] <= splitting_plane) 
            {
                left_primitive_aabbs.push_back(primitive);
                left_aabb.expand_bounds(primitive.aabb);
            } 
            // The entire primitive is on the right side of the splitting plane
            else if (primitive.aabb.min_bounds[info.split.axis] >= splitting_plane) 
            { 
                right_primitive_aabbs.push_back(primitive); 
                right_aabb.expand_bounds(primitive.aabb);
            } 
            // The primitive lies on the border and needs to be split
            else 
            { 
                border_primitive_aabbs.push_back(primitive);
            }
        }

        for(const auto & border_primitive : border_primitive_aabbs)
        {
            AABB expanded_left_aabb = left_aabb;
            AABB expanded_right_aabb = right_aabb;
            AABB dummy_aabb;
            // because of how project primitive into bin is written we need three projections here
            const f32 offset = 1000.0f;
            project_primitive_into_bin({
                .triangle = *border_primitive.primitive,
                .splitting_axis = info.split.axis,
                .left_plane_axis_coord = parent_node.bounding_box.min_bounds[info.split.axis] - offset,
                .right_plane_axis_coord = parent_node.bounding_box.min_bounds[info.split.axis],
                .parent_aabb = parent_node.bounding_box,
                .left_aabb = dummy_aabb,
                .right_aabb = expanded_left_aabb
            });

            project_primitive_into_bin({
                .triangle = *border_primitive.primitive,
                .splitting_axis = info.split.axis,
                .left_plane_axis_coord = parent_node.bounding_box.min_bounds[info.split.axis],
                .right_plane_axis_coord = splitting_plane,
                .parent_aabb = parent_node.bounding_box,
                .left_aabb = expanded_left_aabb,
                .right_aabb = expanded_right_aabb
            });

            project_primitive_into_bin({
                .triangle = *border_primitive.primitive,
                .splitting_axis = info.split.axis,
                .left_plane_axis_coord = splitting_plane,
                .right_plane_axis_coord = parent_node.bounding_box.max_bounds[info.split.axis],
                .parent_aabb = parent_node.bounding_box,
                .left_aabb = expanded_right_aabb,
                .right_aabb = expanded_right_aabb
            });

            SAHCalculateInfo sah_calc_info = SAHCalculateInfo{
                .left_primitive_count = static_cast<u32>(left_primitive_aabbs.size() + 1),
                .right_primitive_count = static_cast<u32>(right_primitive_aabbs.size() + 1),
                .left_aabb_area = expanded_left_aabb.get_area(),
                .right_aabb_area = expanded_right_aabb.get_area(),
                .parent_aabb_area = parent_node.bounding_box.get_area(),
                .ray_aabb_test_cost = info.ray_aabb_intersection_cost,
                .ray_tri_test_cost = info.ray_primitive_intersection_cost
            };
            f32 split_cost = SAH(sah_calc_info);

            AABB unsplit_left_aabb = left_aabb;
            unsplit_left_aabb.expand_bounds(border_primitive.aabb); 
            sah_calc_info.left_primitive_count = static_cast<u32>(left_primitive_aabbs.size() + 1);
            sah_calc_info.right_primitive_count = static_cast<u32>(right_primitive_aabbs.size());
            sah_calc_info.left_aabb_area = unsplit_left_aabb.get_area();
            sah_calc_info.right_aabb_area = right_aabb.get_area();
            f32 unsplit_left_cost = SAH(sah_calc_info);

            AABB unsplit_right_aabb = right_aabb;
            unsplit_right_aabb.expand_bounds(border_primitive.aabb); 
            sah_calc_info.left_primitive_count = static_cast<u32>(left_primitive_aabbs.size());
            sah_calc_info.right_primitive_count = static_cast<u32>(right_primitive_aabbs.size() + 1);
            sah_calc_info.left_aabb_area = left_aabb.get_area();
            sah_calc_info.right_aabb_area = unsplit_right_aabb.get_area();
            f32 unsplit_right_cost = SAH(sah_calc_info);

            f32 min_sah = glm::min(glm::min(unsplit_right_cost, unsplit_left_cost), split_cost);
            if(min_sah == split_cost)
            {
                AABB clipped_left_primitive_aabb = get_intersection_aabb(expanded_left_aabb, border_primitive.aabb);
                AABB clipped_right_primitive_aabb = get_intersection_aabb(expanded_right_aabb, border_primitive.aabb);
                if(expanded_left_aabb.check_if_valid())
                {
                    left_aabb = expanded_left_aabb;
                    assert(clipped_left_primitive_aabb.max_bounds != f32vec3(-INFINITY) && clipped_left_primitive_aabb.min_bounds != f32vec3(INFINITY));
                    left_primitive_aabbs.push_back(PrimitiveAABB(clipped_left_primitive_aabb, border_primitive.primitive));
                }
                if(expanded_right_aabb.check_if_valid())
                {
                    right_aabb = expanded_right_aabb;
                    assert(clipped_right_primitive_aabb.max_bounds != f32vec3(-INFINITY) && clipped_right_primitive_aabb.min_bounds != f32vec3(INFINITY));
                    right_primitive_aabbs.push_back(PrimitiveAABB(clipped_right_primitive_aabb, border_primitive.primitive));
                }
            }
            else if(min_sah == unsplit_left_cost && unsplit_left_aabb.check_if_valid())
            {
                left_aabb = unsplit_left_aabb;
                left_primitive_aabbs.push_back(border_primitive);
            }
            else if (min_sah == unsplit_right_cost && unsplit_right_aabb.check_if_valid())
            {
                right_aabb = unsplit_right_aabb;   
                right_primitive_aabbs.push_back(border_primitive);
            }
            else{
                // never should happen
                assert(false);
            }
        }

        auto & left_child = bvh_nodes.emplace_back();
        left_child.bounding_box = left_aabb;
#ifdef VISUALIZE_SPATIAL_SPLITS
        left_child.spatial = 1u;
#endif
        bvh_nodes.at(info.node_idx).left_index = i32(bvh_nodes.size() - 1);

        auto & right_child = bvh_nodes.emplace_back();
        right_child.bounding_box = right_aabb;
#ifdef VISUALIZE_SPATIAL_SPLITS
        right_child.spatial = 1u;
#endif
        bvh_nodes.at(info.node_idx).right_index = i32(bvh_nodes.size() - 1);



        return {left_primitive_aabbs, right_primitive_aabbs};
    };

    switch(info.split.type)
    {
        case SplitType::OBJECT:
        {
            object_split();

            // Assume the list is sorted (using the splitting axis) - this is done in the object_split() function
            i32 split_event = std::get<i32>(info.split.event);
            std::vector<PrimitiveAABB> left_primitive_aabbs;
            std::vector<PrimitiveAABB> right_primitive_aabbs;
            left_primitive_aabbs.reserve(split_event);
            right_primitive_aabbs.reserve(info.primitive_aabbs.size() - split_event);
            for(int i = 0; i < split_event; i++)
            {
                if(info.primitive_aabbs.at(i).aabb.check_if_valid())
                {
                    left_primitive_aabbs.emplace_back(info.primitive_aabbs.at(i));
                }else
                {
                    DEBUG_OUT("discarding primitive");
                }
            }
            for(int i = split_event; i < info.primitive_aabbs.size(); i++)
            {
                if(info.primitive_aabbs.at(i).aabb.check_if_valid())
                {
                    right_primitive_aabbs.emplace_back(info.primitive_aabbs.at(i));
                }
                else
                {
                    DEBUG_OUT("discarding primitive");
                }
            }
            return {left_primitive_aabbs, right_primitive_aabbs};
        }
        case SplitType::SPATIAL:
        {
            return spatial_split();
        }
    }
    return {};
}

static i32 leaf_count = 0;
auto BVH::construct_bvh_from_data(const std::vector<Triangle> & primitives, const ConstructBVHInfo & info) -> void
{
    bvh_nodes.clear();
    bvh_leaves.clear();
    leaf_count = 0;
    // Generate vector of Primitive AABBs from the vector of primitives
    std::vector<PrimitiveAABB> primitive_aabbs;
    primitive_aabbs.reserve(primitives.size());
    for(int i = 0; i < primitives.size(); i++)
    {
        primitive_aabbs.emplace_back(PrimitiveAABB{
            .aabb = AABB(primitives.at(i)),
            .primitive = &primitives.at(i)
        });
    }

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

        if(primitive_aabbs.size() == 1) 
        { 
            create_leaf({
                .node_idx = node_idx,
                .primitives = primitive_aabbs
            }); 
            nodes.pop();
            continue;
        }

        BestSplitInfo best_split = SAH_greedy_best_split({
            .ray_primitive_cost = info.ray_primitive_intersection_cost,
            .ray_aabb_test_cost = info.ray_aabb_intersection_cost,
            .node_idx = node_idx,
            .primitive_aabbs = primitive_aabbs,
            .join_leaves = info.join_leaves
        });

        if(best_split.axis == Axis::LAST)
        {
            DEBUG_OUT("Early leaf with primitive count " + std::to_string(primitive_aabbs.size()) + " node index " + std::to_string(node_idx));
            create_leaf({
                .node_idx = node_idx,
                .primitives = primitive_aabbs
            });
            nodes.pop();
            continue;
        }

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
                BestSplitInfo spatial_split = spatial_best_split({
                    .ray_primitive_cost = info.ray_primitive_intersection_cost,
                    .ray_aabb_test_cost = info.ray_aabb_intersection_cost,
                    .bin_count = info.spatial_bin_count,
                    .node_idx = node_idx,
                    .primitive_aabbs = primitive_aabbs
                });
                if(spatial_split.cost < best_split.cost) 
                { 
                    best_split = spatial_split; 
                }
            } 
        }

#ifdef LOG_DEBUG
        if( best_split.type == SplitType::OBJECT && std::get<i32>(best_split.event) == 0 )
        {
            DEBUG_OUT("best object event unsplit with primitive count " + std::to_string(primitive_aabbs.size()) + " node index " + std::to_string(node_idx));
        } 
        else if (best_split.type == SplitType::SPATIAL && 
                (std::get<f32>(best_split.event) == bvh_nodes.at(node_idx).bounding_box.max_bounds[best_split.axis]))
        {
            DEBUG_OUT("best spatial event unsplit with primitive count " + std::to_string(primitive_aabbs.size()) + " node index " + std::to_string(node_idx));
        }
#endif

        auto [left_primitive_aabbs, right_primitive_aabbs] = split_node({
            .split = best_split,
            .primitive_aabbs = primitive_aabbs,
            .node_idx = node_idx,
            .ray_primitive_intersection_cost = info.ray_primitive_intersection_cost,
            .ray_aabb_intersection_cost = info.ray_aabb_intersection_cost
        });

        if(left_primitive_aabbs.size() >= 1) {nodes.emplace(bvh_nodes.at(node_idx).left_index, std::move(left_primitive_aabbs));}
        if(right_primitive_aabbs.size() >= 1) {nodes.emplace(bvh_nodes.at(node_idx).right_index, std::move(right_primitive_aabbs));}
        nodes.pop();
    }
    DEBUG_OUT("BVH Built - triangle count: " + std::to_string(primitives.size()) + " leaf count: " + std::to_string(bvh_leaves.size()) + " primitives in leaves " + std::to_string(leaf_count));
}

auto BVH::create_leaf(const CreateLeafInfo & info) -> void
{
    auto & leaf = bvh_leaves.emplace_back();
    leaf_count += info.primitives.size();
    for(const auto & primitiveAABB : info.primitives)
    {
        leaf.primitives.push_back(primitiveAABB.primitive);
    }
    bvh_nodes.at(info.node_idx).left_index = -1;
    bvh_nodes.at(info.node_idx).right_index = i32(bvh_leaves.size() - 1);
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
            .depth = static_cast<daxa::u32>(depth),
#ifdef VISUALIZE_SPATIAL_SPLITS
            .spatial = node.spatial
#endif
        });
        
        if(node.left_index > 0) 
        {
            que.push({depth + 1u, bvh_nodes.at(node.left_index)});
            if(node.right_index != 0)
            {
                que.push({depth + 1u, bvh_nodes.at(node.right_index)});
            }
            else 
            { 
                // NOTE(msakmary) should not happen
                assert(false);
            }
        }
#ifdef VISUALIZE_SPATIAL_SPLITS
        else if (node.left_index < 0) { info.back().spatial = 2; }
#endif
        else 
        {
            // NOTE(msakmary) should not happen
            assert(false);
        }
        que.pop();
    }
    return info;
}

auto BVH::get_nearest_intersection(const Ray & ray) const -> Hit
{
    const auto & root_node = bvh_nodes.at(0);

    auto hit = root_node.bounding_box.ray_box_intersection(ray);
    // The ray missed the scene
    if(!hit.hit) { return hit; }

    Hit nearest_hit = Hit {
        .hit = false,
        .distance = INFINITY,
        .normal = f32vec3(0.0f, 0.0f, 0.0f),
        .internal_fac = 1.0f
    };
    // Node consists of the bvh_node index and the intersection distance
    using Node = std::pair<i32, f32>;
    auto comparator = [](const Node & first, const Node & second) -> bool
    {
        return first.second > second.second;
    };

    std::priority_queue<Node, std::vector<Node>, decltype(comparator)> nodes_queue(comparator);

    // NOTE(msakmary) Why would you have bvh with two leaves?? STOP
    assert(root_node.left_index > 0 && root_node.right_index > 0);

    hit = bvh_nodes.at(root_node.left_index).bounding_box.ray_box_intersection(ray);
    if(hit.hit) { nodes_queue.emplace(root_node.left_index, hit.distance); }
    hit = bvh_nodes.at(root_node.right_index).bounding_box.ray_box_intersection(ray);
    if(hit.hit) { nodes_queue.emplace(root_node.right_index, hit.distance); }

    while(!nodes_queue.empty())
    {
        auto [node_idx, intersect_distance] = nodes_queue.top();
        nodes_queue.pop();
        // nearest AABB intersection is farther than nearest primitive hit, stop tracing
        // if(intersect_distance > nearest_hit.distance) { break; }

        const auto curr_node = bvh_nodes.at(node_idx);

        // Nodes are not leaves so find the intersection and add it to the queue for processing
        if(curr_node.left_index > 0)
        {
            hit = bvh_nodes.at(curr_node.left_index).bounding_box.ray_box_intersection(ray);
            if(hit.hit) { nodes_queue.emplace(curr_node.left_index, hit.distance); }

            if(curr_node.right_index > 0)
            {
                hit = bvh_nodes.at(curr_node.right_index).bounding_box.ray_box_intersection(ray);
                if(hit.hit) { nodes_queue.emplace(curr_node.right_index, hit.distance); }
            }
        }

        // Node is a leaf intersect all primitives and compare the intersections with the best hit found so far
        if(curr_node.left_index == -1)
        {
            const auto & leaf = bvh_leaves.at(curr_node.right_index);

            for(const Triangle * leaf_primitive : leaf.primitives)
            {
                auto leaf_hit = leaf_primitive->intersect_ray(ray);
                if(leaf_hit.hit && leaf_hit.distance < nearest_hit.distance)
                {
                    nearest_hit = leaf_hit;
                }
            }
        }
    }
    return nearest_hit;
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
