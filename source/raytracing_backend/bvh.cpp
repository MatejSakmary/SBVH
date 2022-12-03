#include "bvh.hpp"

AABB::AABB() : left_bottom_front(INFINITY), right_top_back(-INFINITY) {}

AABB::AABB(const f32vec3 & left_bottom_front, const f32vec3 & right_top_back) :
    left_bottom_front{left_bottom_front}, right_top_back{right_top_back} {}

void AABB::expand_bounds(const f32vec3 & vertex)
{
    left_bottom_front = component_wise_min(left_bottom_front, vertex);
    right_top_back = component_wise_max(right_top_back, vertex);
};

void BVH::construct_bvh_from_data(const std::vector<Triangle> & primitives)
{
    auto & node = bvh_nodes.emplace_back();
    for(const auto & primitive : primitives)
    {
        node.bounding_box.expand_bounds(primitive.v0);
        node.bounding_box.expand_bounds(primitive.v1);
        node.bounding_box.expand_bounds(primitive.v2);
    }
}

auto BVH::get_bvh_visualization_data() const -> std::vector<AABBGeometryInfo>
{
    std::vector<AABBGeometryInfo> info;

    const auto & aabb = bvh_nodes.at(0).bounding_box;
    info.emplace_back(AABBGeometryInfo{
        .position = daxa_vec3_from_glm(aabb.left_bottom_front),
        .scale = daxa_vec3_from_glm(aabb.right_top_back - aabb.left_bottom_front)
    });
    return info;
}