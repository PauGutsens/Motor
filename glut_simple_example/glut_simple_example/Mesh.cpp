#include "Mesh.h"
#include <cstddef>

Mesh::Mesh(const std::vector<Vertex>& verts, const std::vector<unsigned int>& inds)
    : vertices(verts), indices(inds) {
}

Mesh::~Mesh() {
    cleanup();
}

void Mesh::setupMesh() {
    if (_isSetup || vertices.empty()) return;

    // Generate and bind VAO/VBO/EBO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
        vertices.data(),
        GL_STATIC_DRAW);

    if (!indices.empty()) {
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
            indices.data(),
            GL_STATIC_DRAW);
    }

    // Fixed pipeline client array setup
    // Vertex coordinates: using double precision, consistent with vec3 (glm::dvec3) in types.h
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_DOUBLE, sizeof(Vertex),
        reinterpret_cast<const void*>(offsetof(Vertex, position)));

    // Normals: also double precision
    glEnableClientState(GL_NORMAL_ARRAY);
    glNormalPointer(GL_DOUBLE, sizeof(Vertex),
        reinterpret_cast<const void*>(offsetof(Vertex, normal)));

    // Texture coordinates: float2 (glm::vec2)
    glClientActiveTexture(GL_TEXTURE0);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex),
        reinterpret_cast<const void*>(offsetof(Vertex, texCoord)));

    // Unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (!indices.empty()) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    _isSetup = true;
}

void Mesh::draw() const {
    if (!_isSetup || VAO == 0) return;

    // Fixed pipeline: enable and bind texture only when present
    if (textureID != 0) {
        glEnable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
    }

    // Bind VAO and draw
    glBindVertexArray(VAO);

    if (!indices.empty()) {
        glDrawElements(GL_TRIANGLES,
            static_cast<GLsizei>(indices.size()),
            GL_UNSIGNED_INT, 0);
    }
    else {
        glDrawArrays(GL_TRIANGLES, 0,
            static_cast<GLsizei>(vertices.size()));
    }

    glBindVertexArray(0);

    if (textureID != 0) {
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    }

    // Draw normals if enabled
    if (showVertexNormals) drawVertexNormals();
    if (showFaceNormals) drawFaceNormals();
}

void Mesh::drawVertexNormals() const {
    glDisable(GL_LIGHTING);
    glColor3f(0.0f, 1.0f, 0.0f); // Green for vertex normals
    glBegin(GL_LINES);

    for (const auto& v : vertices) {
        vec3 start = v.position;
        vec3 end = v.position + v.normal * normalLength;
        glVertex3d(start.x, start.y, start.z);
        glVertex3d(end.x, end.y, end.z);
    }

    glEnd();
    glEnable(GL_LIGHTING);
}

void Mesh::drawFaceNormals() const {
    glDisable(GL_LIGHTING);
    glColor3f(0.0f, 0.0f, 1.0f); // Blue for face normals
    glBegin(GL_LINES);

    size_t triCount = getTriangleCount();
    for (size_t i = 0; i < triCount; ++i) {
        // Get triangle vertices
        vec3 v0, v1, v2;
        if (!indices.empty()) {
            v0 = vertices[indices[i * 3 + 0]].position;
            v1 = vertices[indices[i * 3 + 1]].position;
            v2 = vertices[indices[i * 3 + 2]].position;
        }
        else {
            v0 = vertices[i * 3 + 0].position;
            v1 = vertices[i * 3 + 1].position;
            v2 = vertices[i * 3 + 2].position;
        }

        // Calculate face center
        vec3 center = (v0 + v1 + v2) / 3.0;

        // Calculate face normal
        vec3 edge1 = v1 - v0;
        vec3 edge2 = v2 - v0;
        vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        // Draw normal line
        vec3 end = center + normal * normalLength;
        glVertex3d(center.x, center.y, center.z);
        glVertex3d(end.x, end.y, end.z);
    }

    glEnd();
    glEnable(GL_LIGHTING);
}

unsigned int Mesh::getTextureID() const {
    return textureID;
}

void Mesh::cleanup() {
    if (EBO) { glDeleteBuffers(1, &EBO); EBO = 0; }
    if (VBO) { glDeleteBuffers(1, &VBO); VBO = 0; }
    if (VAO) { glDeleteVertexArrays(1, &VAO); VAO = 0; }
}