#pragma once

#include "types.h"
#include <vector>
#include <string>
#include <GL/glew.h>

struct Vertex {
    vec3 position;
    vec3 normal;
    glm::vec2 texCoord;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    GLuint textureID = 0;

    Mesh() = default;
    Mesh(const std::vector<Vertex>& verts, const std::vector<unsigned int>& inds);
    ~Mesh();

    void setupMesh();
    void draw() const;
    void setTexture(GLuint texID);
    void cleanup();

private:
    bool _isSetup = false;
};