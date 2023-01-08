#include "scene.hpp"

#include <stack>
#include <string>

void Scene::process_mesh(const ProcessMeshInfo & info)
{
    auto & new_mesh = info.object.meshes.emplace_back(RuntimeMesh{});
    new_mesh.vertices.reserve(info.mesh->mNumVertices);
    new_mesh.indices.reserve(static_cast<std::vector<u32>::size_type>(info.mesh->mNumFaces) * 3); // expect triangles
    for(u32 vertex = 0; vertex < info.mesh->mNumVertices; vertex++)
    {
        f32vec3 pre_transform_position{ 
            info.mesh->mVertices[vertex].x,
            info.mesh->mVertices[vertex].y,
            info.mesh->mVertices[vertex].z
        };
        f32vec3 pre_transform_normal{
            info.mesh->mNormals[vertex].x,
            info.mesh->mNormals[vertex].y,
            info.mesh->mNormals[vertex].z
        };

        // Runtime rendering data
        new_mesh.vertices.emplace_back(Vertex{
            .position = pre_transform_position,
            .normal = pre_transform_normal,
        });
    }

    raytracing_scene.primitives.reserve(raytracing_scene.primitives.size() + info.mesh->mNumFaces);
    f32mat4x4 m_model = info.object.transform;

    // NOTE(msakmary) I am assuming triangles here
    for(u32 face = 0; face < info.mesh->mNumFaces; face++)
    {
        aiFace face_obj = info.mesh->mFaces[face];

        new_mesh.indices.emplace_back(face_obj.mIndices[0]);
        new_mesh.indices.emplace_back(face_obj.mIndices[1]);
        new_mesh.indices.emplace_back(face_obj.mIndices[2]);

        const auto v0 = info.mesh->mVertices[face_obj.mIndices[0]];
        const auto v1 = info.mesh->mVertices[face_obj.mIndices[1]];
        const auto v2 = info.mesh->mVertices[face_obj.mIndices[2]];
            
        f32vec4 pre_transform_v0{ v0.x, v0.y, v0.z, 1.0f };
        f32vec4 pre_transform_v1{ v1.x, v1.y, v1.z, 1.0f };
        f32vec4 pre_transform_v2{ v2.x, v2.y, v2.z, 1.0f };

        f32vec3 post_transform_v0 = m_model * pre_transform_v0;
        f32vec3 post_transform_v1 = m_model * pre_transform_v1;
        f32vec3 post_transform_v2 = m_model * pre_transform_v2;

        f32vec3 normal{
            glm::normalize(
                glm::cross(
                    post_transform_v1 - post_transform_v0, 
                    post_transform_v2 - post_transform_v0
                )
            )
        };
        // Raytracing data
        raytracing_scene.primitives.emplace_back(Triangle{
            .v0 = post_transform_v0,
            .v1 = post_transform_v1,
            .v2 = post_transform_v2,
            .normal = normal
        });
    }
}

void Scene::process_scene(const aiScene * scene)
{
    auto mat_assimp_to_glm = [](const aiMatrix4x4 & mat) -> f32mat4x4
    {
        return {{mat.a1, mat.b1, mat.c1, mat.d1},
                {mat.a2, mat.b2, mat.c2, mat.d2},
                {mat.a3, mat.b3, mat.c3, mat.d3},
                {mat.a4, mat.b4, mat.c4, mat.d4}};
    };
    using node_element = std::tuple<const aiNode *, aiMatrix4x4>; 
    std::stack<node_element> node_stack;

    node_stack.push({scene->mRootNode, aiMatrix4x4()});

    while(!node_stack.empty())
    {
        auto [node, parent_transform] = node_stack.top();

        if(node->mNumMeshes > 0)
        {
            auto & new_scene_object = runtime_scene_objects.emplace_back(RuntimeSceneObject{
                .transform = mat_assimp_to_glm(parent_transform)
            });

            for(u32 i = 0; i < node->mNumMeshes; i++)
            {
                process_mesh({
                    .mesh = scene->mMeshes[node->mMeshes[i]],
                    .scene = scene,
                    .object = new_scene_object
                });
            }
        }
        node_stack.pop();
        for(int i = 0; i < node->mNumChildren; i++)
        {
            auto *child = node->mChildren[i];
            auto node_transform = parent_transform * child->mTransformation;
            node_stack.push({child, node_transform});
        }
    }
};

Scene::Scene(const std::string & scene_path)
{
    Assimp::Importer importer;
    const aiScene * scene = importer.ReadFile( 
        scene_path, 
        aiProcess_Triangulate           |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType);

    if((scene == nullptr) || ((scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0u) || (scene->mRootNode == nullptr))
    {
        std::string err_string = importer.GetErrorString();
        DEBUG_OUT("[Scene::Scene()] Error assimp");
        DEBUG_OUT(err_string);
        return;
    }

    process_scene(scene);
}

void Scene::build_bvh(const BuildBVHInfo & info)
{
    raytracing_scene.bvh.construct_bvh_from_data(
        raytracing_scene.primitives, ConstructBVHInfo{
            .ray_primitive_intersection_cost = 2.0f,
            .ray_aabb_intersection_cost = 3.0f,
            .spatial_bin_count = 128u,
            .spatial_alpha = 1.0f
        }
    );
}