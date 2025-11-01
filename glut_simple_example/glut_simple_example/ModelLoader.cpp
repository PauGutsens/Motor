#include "ModelLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <IL/il.h>
#include <IL/ilu.h>
#include <iostream>
#include <filesystem>
#include <assimp/material.h>
#include "TextureLoader.h"
#include "Mesh.h"            // 需要 setTexture()
#include <filesystem>        // 用来拼接目录




std::vector<std::shared_ptr<Mesh>> ModelLoader::loadModel(const std::string& path) {
    std::vector<std::shared_ptr<Mesh>> meshes;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return meshes;
    }

    std::cout << "Loading model: " << path << std::endl;
    std::cout << "Number of meshes: " << scene->mNumMeshes << std::endl;

    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];
        auto processedMesh = processMesh(mesh, scene);
        if (processedMesh) {
            meshes.push_back(processedMesh);
        }
    }

    return meshes;
}

std::shared_ptr<Mesh> ModelLoader::processMesh(void* meshPtr, const void* scenePtr) {
    aiMesh* mesh = static_cast<aiMesh*>(meshPtr);
    const aiScene* scene = static_cast<const aiScene*>(scenePtr);

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;

        // Position
        vertex.position.x = mesh->mVertices[i].x;
        vertex.position.y = mesh->mVertices[i].y;
        vertex.position.z = mesh->mVertices[i].z;

        // Normals
        if (mesh->HasNormals()) {
            vertex.normal.x = mesh->mNormals[i].x;
            vertex.normal.y = mesh->mNormals[i].y;
            vertex.normal.z = mesh->mNormals[i].z;
        }
        else {
            vertex.normal = vec3(0.0, 1.0, 0.0);
        }

        // Texture coordinates
        if (mesh->mTextureCoords[0]) {
            vertex.texCoord.x = mesh->mTextureCoords[0][i].x;
            vertex.texCoord.y = mesh->mTextureCoords[0][i].y;
        }
        else {
            vertex.texCoord = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    // Process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    auto resultMesh = std::make_shared<Mesh>(vertices, indices);
    resultMesh->setupMesh();

    return resultMesh;
}

GLuint ModelLoader::loadTexture(const std::string& path) {
    static bool ilInitialized = false;
    if (!ilInitialized) {
        ilInit();
        iluInit();
        ilInitialized = true;
    }

    ILuint imageID;
    ilGenImages(1, &imageID);
    ilBindImage(imageID);

    if (!ilLoadImage(path.c_str())) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        std::cerr << "DevIL error: " << iluErrorString(ilGetError()) << std::endl;
        ilDeleteImages(1, &imageID);
        return 0;
    }

    // Convert image to RGBA format
    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        ilGetInteger(IL_IMAGE_WIDTH),
        ilGetInteger(IL_IMAGE_HEIGHT),
        0, GL_RGBA, GL_UNSIGNED_BYTE,
        ilGetData());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenerateMipmap(GL_TEXTURE_2D);

    ilDeleteImages(1, &imageID);

    std::cout << "Loaded texture: " << path << " (ID: " << textureID << ")" << std::endl;

    return textureID;
}
static std::string DirName(const std::string& file) {
    std::filesystem::path p(file);
    return p.has_parent_path() ? p.parent_path().string() : std::string(".");
}

// 给 mesh 设置漫反射贴图（若材质有）
static void AssignDiffuseTextureIfAny(const aiMaterial* mat,
    const std::string& modelPath,
    const std::shared_ptr<Mesh>& mesh)
{
    if (!mat || !mesh) return;

    aiString tex;
    if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &tex) == AI_SUCCESS) {
        std::string base = DirName(modelPath);
        std::string full = (std::filesystem::path(base) / tex.C_Str()).string();

        if (unsigned int t = LoadTexture2D(full)) {
            mesh->setTexture(t);
        }
        // 没找到就保持0，不影响绘制（仅无纹理）
    }
}
