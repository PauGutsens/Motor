#include "ModelLoader.h"
#include "TextureLoader.h"
#include "Mesh.h"
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
    static void AssignDiffuseTextureIfAny(const aiMaterial* mat,
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
    }

}

std::shared_ptr<Mesh> ModelLoader::processMesh(void* meshPtr, const void* scenePtr) {
    aiMesh* mesh = static_cast<aiMesh*>(meshPtr);
    if (!mesh) return nullptr;
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

unsigned int ModelLoader::loadTexture(const std::string& path) {
    EnsureDevILInited();
    return LoadTexture2D(path);
}