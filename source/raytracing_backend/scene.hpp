#pragma once

#include <string>
#include <vector>
#include <utility>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "../types.hpp"
#include "triangle.hpp"
#include "bvh.hpp"

struct Vertex
{
    f32vec3 position;
    f32vec3 normal;
};

struct RuntimeMesh
{
    std::vector<u32> indices;
    std::vector<Vertex> vertices;
};

// Scene object used in real time visualisation
struct RuntimeSceneObject
{
    f32mat4x4 transform;
    std::vector<RuntimeMesh> meshes;
};

// Scene object used in raytracing
struct RaytracingScene
{
    std::vector<Triangle> primitives;
    BVH bvh;
};

struct ProcessMeshInfo
{
    const aiMesh * mesh;
    const aiScene * scene;
    RuntimeSceneObject & object;
};

struct Scene
{
    std::vector<RuntimeSceneObject> runtime_scene_objects;
    RaytracingScene raytracing_scene;

    explicit Scene(const std::string & scene_path);
    void build_bvh(const ConstructBVHInfo & info);

    private:
        void process_scene(const aiScene * scene);
        void process_mesh(const ProcessMeshInfo & info);
        void convert_to_raytrace_scene();
};