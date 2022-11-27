#include "scene.hpp"

#include <stack>

void Scene::process_mesh(const ProcessMeshInfo & info)
{
    auto & new_mesh = info.object.meshes.emplace_back(RuntimeMesh{});
    new_mesh.vertices.reserve(info.mesh->mNumVertices);
    new_mesh.indices.reserve(info.mesh->mNumFaces);

    for(u32 vertex = 0; vertex < info.mesh->mNumVertices; vertex++)
    {
        new_mesh.vertices.emplace_back(Vertex{
            .position = {info.mesh->mVertices[vertex].x,
                         info.mesh->mVertices[vertex].y,
                         info.mesh->mVertices[vertex].z},
            .normal =   {info.mesh->mNormals[vertex].x,
                         info.mesh->mNormals[vertex].y,
                         info.mesh->mNormals[vertex].z}
        });
    }

    for(u32 face = 0; face < info.mesh->mNumFaces; face++)
    {
        aiFace face_obj = info.mesh->mFaces[face];
        for(u32 index = 0; index < face_obj.mNumIndices; index++)
        {
            new_mesh.indices.emplace_back(face_obj.mIndices[index]);
        }
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
            auto child = node->mChildren[i];
            auto node_transform = parent_transform * node->mTransformation;
            node_stack.push({child, node_transform});
        }
    }
};

Scene::Scene(const std::string scene_path)
{
    Assimp::Importer importer;
    const aiScene * scene = importer.ReadFile( 
        scene_path, 
        aiProcess_Triangulate           |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType);

    if((!scene) || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || (!scene->mRootNode))
    {
        DEBUG_OUT("[Scene::Scene()] Error assimp " << importer.GetErrorString());
        return;
    }

    process_scene(scene);
}