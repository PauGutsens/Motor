#pragma once
#include "types.h"
#include <vector>
#include <string>
#include <GL/glew.h>

struct Vertex {
    vec3       position;
    vec3       normal;
    glm::vec2  texCoord;
};

class Mesh {
public:
    std::vector<Vertex>         vertices;
    std::vector<unsigned int>   indices;
    unsigned int getTextureID() const;

    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    GLuint textureID = 0;

    bool showVertexNormals = false;
    bool showFaceNormals = false;
    double normalLength = 0.2;

    Mesh() = default;
    Mesh(const std::vector<Vertex>& verts, const std::vector<unsigned int>& inds);
    ~Mesh();
    void setupMesh();
    void draw() const;
    void cleanup();
    void setTexture(GLuint texID) { textureID = texID; }
    unsigned int getTexture() const { return textureID; }
    size_t getVertexCount() const { return vertices.size(); }
    size_t getTriangleCount() const { return indices.empty() ? vertices.size() / 3 : indices.size() / 3; }

private:
    bool _isSetup = false;
    void drawVertexNormals() const;
    void drawFaceNormals() const;
};