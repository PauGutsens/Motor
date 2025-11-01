#pragma once

#include "types.h"
#include <vector>
#include <string>
#include <GL/glew.h>

// 顶点：位置/法线为双精度 vec3（与你的 types.h 一致），UV 为 float2 Vertice: La posicion/normal es vec3 de doble precision (consistente con types.h), UV es float2.
struct Vertex {
    vec3       position;
    vec3       normal;
    glm::vec2  texCoord;
};

class Mesh {
public:
    std::vector<Vertex>         vertices;
    std::vector<unsigned int>   indices;

    // GPU 资源
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;

    // 统一仅保留一个纹理字段 Solo se conserva un campo de textura.
    GLuint textureID = 0;

    Mesh() = default;
    Mesh(const std::vector<Vertex>& verts, const std::vector<unsigned int>& inds);
    ~Mesh();

    void setupMesh();
    void draw() const;
    void cleanup();

    // 统一接口 Interfaz unificada
    void setTexture(GLuint texID) { textureID = texID; }
    unsigned int getTexture() const { return textureID; }

private:
    bool _isSetup = false;
};
