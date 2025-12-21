#include "ModelLoader.h"
#include "TextureLoader.h"
#include "Mesh.h"
#include "MeshImporter.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

#include <GL/glew.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using std::shared_ptr;
using std::make_shared;
using std::string;

namespace fs = std::filesystem;

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

// Return directory of given path (or "." if no parent)
// 返回给定路径的目录部分（如果没有父目录，则返回 ".")
static inline fs::path DirName(const fs::path& p)
{
    return p.has_parent_path() ? p.parent_path() : fs::path(".");
}

// 尝试从 Assimp 材质中读取漫反射贴图并绑定到 Mesh
// Try to assign a diffuse texture from an Assimp material to a mesh
static void AssignDiffuseTextureIfAny(const aiMaterial* mat,
    const std::string& modelPath,
    const shared_ptr<Mesh>& mesh)
{
    if (!mat || !mesh)
        return;

    aiString tex;
    if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &tex) != AI_SUCCESS || tex.length == 0)
        return;

    EnsureDevILInited();

    fs::path modelDir = DirName(fs::path(modelPath));
    fs::path texPath(tex.C_Str());

    // 构造一组候选路径，尽量兼容 "..\\textures\\xxx.tga" 这种写法
    // Build a list of candidate paths where the texture may live.
    std::vector<fs::path> candidates;

    if (texPath.is_absolute())
    {
        // 材质里已经是绝对路径
        candidates.push_back(texPath);
    }
    else
    {
        // 1) 模型所在目录 + 原始相对路径
        //    e.g. Street/..\textures/a.tga -> Assets/textures/a.tga
        candidates.push_back(modelDir / texPath);

        // 2) 模型所在目录 + 文件名
        //    针对“所有贴图都丢到 FBX 同目录”的情况
        candidates.push_back(modelDir / texPath.filename());

        // 3) 模型所在目录下的 textures 子目录 + 文件名
        candidates.push_back(modelDir / "textures" / texPath.filename());
    }

    unsigned int texId = 0;
    for (const fs::path& c : candidates)
    {
        if (!fs::exists(c))
            continue;

        texId = LoadTexture2D(c.string());
        if (texId != 0)
        {
            mesh->setTexture(texId);
            return;
        }
    }

    // 最后兜底再尝试一次原字符串（以防它是相对于当前工作目录的路径）
    if (texId == 0)
    {
        texId = LoadTexture2D(tex.C_Str());
        if (texId != 0)
        {
            mesh->setTexture(texId);
        }
    }
}

// -----------------------------------------------------------------------------
// ModelLoader::processMesh
// -----------------------------------------------------------------------------
std::shared_ptr<Mesh> ModelLoader::processMesh(void* meshPtr, const void* /*scenePtr*/)
{
    aiMesh* mesh = static_cast<aiMesh*>(meshPtr);
    if (!mesh)
        return nullptr;

    // Extract data from Assimp mesh / 从 Assimp mesh 中抽取数据
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    vertices.reserve(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex v{};
        v.position.x = mesh->mVertices[i].x;
        v.position.y = mesh->mVertices[i].y;
        v.position.z = mesh->mVertices[i].z;

        if (mesh->HasNormals())
        {
            v.normal.x = mesh->mNormals[i].x;
            v.normal.y = mesh->mNormals[i].y;
            v.normal.z = mesh->mNormals[i].z;
        }
        else
        {
            v.normal = vec3(0.0, 0.0, 1.0);
        }

        if (mesh->mTextureCoords[0])
        {
            v.texCoord.x = mesh->mTextureCoords[0][i].x;
            v.texCoord.y = mesh->mTextureCoords[0][i].y;
        }
        else
        {
            v.texCoord = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(v);
    }

    // Faces (already triangulated) / 面（三角化后）
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    // -------------------------------------------------------------------------
    // Save to Library as our custom binary format (.myMesh)
    // 保存到 Library 目录
    // -------------------------------------------------------------------------
    std::string meshName = (mesh->mName.length > 0) ? mesh->mName.C_Str() : "mesh";
    fs::path libraryPath = fs::path("Library") / (meshName + ".myMesh");

    MeshImporter::Save(libraryPath.string().c_str(), vertices, indices);

    // 再通过 MeshImporter 从 Library 读回（让 Library 成为唯一的真实来源）
    // Try to load back via MeshImporter (so that Library is the single source of truth)
    auto loadedMesh = MeshImporter::Load(libraryPath.string().c_str());
    if (loadedMesh)
    {
        return loadedMesh;
    }

    // 兜底：如果读取失败，就直接用内存里的数据创建 Mesh
    // Fallback: if for some reason Library load failed, create a Mesh directly.
    auto directMesh = std::make_shared<Mesh>(vertices, indices);
    directMesh->setupMesh();
    directMesh->computeAABB();
    return directMesh;
}

// -----------------------------------------------------------------------------
// ModelLoader::loadModel
// -----------------------------------------------------------------------------
std::vector<std::shared_ptr<Mesh>> ModelLoader::loadModel(const std::string& path)
{
    std::vector<std::shared_ptr<Mesh>> meshes;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path.c_str(),
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_ImproveCacheLocality |
        aiProcess_JoinIdenticalVertices |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_OptimizeMeshes |
        aiProcess_CalcTangentSpace |
        aiProcess_ValidateDataStructure |
        aiProcess_PreTransformVertices
    );

    if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0)
    {
        std::cerr << "Assimp error while loading \"" << path << "\": "
            << importer.GetErrorString() << std::endl;
        return meshes;
    }

    meshes.reserve(scene->mNumMeshes);

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
    {
        aiMesh* am = scene->mMeshes[i];
        if (!am)
            continue;

        auto m = ModelLoader::processMesh(am, scene);
        if (!m)
            continue;

        // 如果材质中有 Diffuse 贴图，就绑定到 Mesh 上
        // Attach diffuse texture if there is one on the material
        if (am->mMaterialIndex < scene->mNumMaterials)
        {
            const aiMaterial* mat = scene->mMaterials[am->mMaterialIndex];
            AssignDiffuseTextureIfAny(mat, path, m);
        }

        meshes.push_back(m);
    }

    return meshes;
}

// -----------------------------------------------------------------------------
// ModelLoader::loadTexture (simple wrapper) / 简单封装
// -----------------------------------------------------------------------------
unsigned int ModelLoader::loadTexture(const std::string& path)
{
    EnsureDevILInited();
    return LoadTexture2D(path);
}
