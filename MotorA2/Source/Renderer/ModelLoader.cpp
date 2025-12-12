#include "ModelLoader.h"
#include "TextureLoader.h"
#include "Mesh.h"
#include "MeshImporter.h" // Incluimos nuestro importer
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <GL/glew.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>
#include <string>

using std::string;
using std::vector;
using std::shared_ptr;
using std::make_shared;
using std::cout;
using std::cerr;
using std::endl;

namespace {
    static inline std::string DirName(const std::string& p) {
        std::filesystem::path fp(p);
        return fp.has_parent_path() ? fp.parent_path().string() : std::string(".");
    }
   /* static void AssignDiffuseTextureIfAny(const aiMaterial* mat,
        const std::string& modelPath,
        const shared_ptr<Mesh>& mesh) {
        if (!mat || !mesh) return;
        aiString tex;
        if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &tex) == AI_SUCCESS) {
            std::filesystem::path full = std::filesystem::path(DirName(modelPath)) / tex.C_Str();
            EnsureDevILInited();
            if (unsigned int t = LoadTexture2D(full.string())) {
                mesh->setTexture(t);
                return;
            }
            if (unsigned int t2 = LoadTexture2D(tex.C_Str())) {
                mesh->setTexture(t2);
            }
        }
    }*/
    // ------------------------------------------------------------
// 移除所有纹理加载逻辑（避免依赖 DevIL）
    static void AssignDiffuseTextureIfAny(const aiMaterial* mat,
        const std::string& directory,
        const std::shared_ptr<Mesh>& mesh)
    {
        // 暂时忽略材质的贴图，不加载纹理
        (void)mat;
        (void)directory;

        // 将来如果要恢复纹理加载，在这里调用 LoadTexture2D()
        mesh->textureID = 0;
    }

}

std::shared_ptr<Mesh> ModelLoader::processMesh(void* meshPtr, const void* scenePtr) {
    aiMesh* mesh = static_cast<aiMesh*>(meshPtr);
    if (!mesh) return nullptr;

    // Extraer datos de Assimp (FBX)
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    vertices.reserve(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        Vertex v{};
        v.position.x = mesh->mVertices[i].x;
        v.position.y = mesh->mVertices[i].y;
        v.position.z = mesh->mVertices[i].z;

        if (mesh->HasNormals()) {
            v.normal.x = mesh->mNormals[i].x;
            v.normal.y = mesh->mNormals[i].y;
            v.normal.z = mesh->mNormals[i].z;
        }
        else {
            v.normal = vec3(0.0, 0.0, 1.0);
        }

        if (mesh->mTextureCoords[0]) {
            v.texCoord.x = mesh->mTextureCoords[0][i].x;
            v.texCoord.y = mesh->mTextureCoords[0][i].y;
        }
        else {
            v.texCoord = glm::vec2(0.0f, 0.0f);
        }
        vertices.push_back(v);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Guardar en formato propio (Library)

    // Generar nombre unico o usar el del mesh
    std::string meshName = (mesh->mName.length > 0) ? mesh->mName.C_Str() : "mesh";
    
    std::string libraryPath = "Library/" + meshName + ".myMesh";

    // Guardar el archivo binario en disco
    MeshImporter::Save(libraryPath.c_str(), vertices, indices);

    // Cargar desde formato propio (Runtime)

    // Cargar usando nuestro sistema (para probar que funciona)
    auto loadedMesh = MeshImporter::Load(libraryPath.c_str());

    // Si carga bien devolvemos ese, si no usamos el de Assimp como backup
    if (loadedMesh) {
        return loadedMesh;
    }

    // Backup por si falla la carga
    auto result = std::make_shared<Mesh>(vertices, indices);
    result->setupMesh();
    return result;
}

std::vector<std::shared_ptr<Mesh>> ModelLoader::loadModel(const std::string& path) {
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
        aiProcess_ValidateDataStructure
    );

    if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0) {
        std::cerr << "Assimp error while loading \"" << path << "\": "
            << importer.GetErrorString() << std::endl;
        return meshes;
    }
    meshes.reserve(scene->mNumMeshes);
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        aiMesh* am = scene->mMeshes[i];

        // Procesar mesh (ahora guarda y carga de library)
        auto m = ModelLoader::processMesh(am, scene);

        if (!m) continue;
        if (am->mMaterialIndex < scene->mNumMaterials) {
            const aiMaterial* mat = scene->mMaterials[am->mMaterialIndex];
            AssignDiffuseTextureIfAny(mat, path, m);
        }
        meshes.push_back(m);
    }
    return meshes;
}

//unsigned int ModelLoader::loadTexture(const std::string& path) {
//    EnsureDevILInited();
//    return LoadTexture2D(path);
//}
unsigned int ModelLoader::loadTexture(const std::string& path) {
    // TODO: 将来在这里调用真正的纹理加载代码（DevIL 或 stb_image）
    // 目前为了让引擎编译通过、先完成 A2 的结构和场景部分，
    // 我们返回 0（相当于“没有纹理”），后续有时间再接贴图系统。
    (void)path; // 防止未使用参数的编译器警告
    return 0;
}
