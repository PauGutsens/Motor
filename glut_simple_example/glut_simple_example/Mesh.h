#pragma once

#include "types.h"
#include <vector>
#include <string>
#include <GL/glew.h>

// ���㣺λ��/����Ϊ˫���� vec3������� types.h һ�£���UV Ϊ float2
struct Vertex {
    vec3       position;
    vec3       normal;
    glm::vec2  texCoord;
};

class Mesh {
public:
    std::vector<Vertex>         vertices;
    std::vector<unsigned int>   indices;

    // GPU ��Դ
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;

    // ͳһ������һ�������ֶ�
    GLuint textureID = 0;

    Mesh() = default;
    Mesh(const std::vector<Vertex>& verts, const std::vector<unsigned int>& inds);
    ~Mesh();

    void setupMesh();
    void draw() const;
    void cleanup();

    // ͳһ�ӿ�
    void setTexture(GLuint texID) { textureID = texID; }
    unsigned int getTexture() const { return textureID; }

private:
    bool _isSetup = false;
};
