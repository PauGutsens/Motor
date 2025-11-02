#include "Mesh.h"
#include <cstddef>


// struct Vertex { vec3 position; vec3 normal; glm::vec2 texCoord; }

Mesh::Mesh(const std::vector<Vertex>& verts, const std::vector<unsigned int>& inds)
    : vertices(verts), indices(inds) {
}

Mesh::~Mesh() {
    cleanup();
}

void Mesh::setupMesh() {
    if (_isSetup || vertices.empty()) return;

    // 生成并绑定 VAO/VBO/EBO Generar y enlazar VAO/VBO/EBO
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

    
    //  固定管线客户端数组设置 Configuracion fija de la matriz de clientes de la canalizacion
    
    // 顶点坐标：使用双精度，与 types.h 的 vec3 (glm::dvec3) 一致Coordenadas de vertices: utilice doble precision, de forma consistente con vec3 (glm::dvec3) en types.h.
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_DOUBLE, sizeof(Vertex),
        reinterpret_cast<const void*>(offsetof(Vertex, position)));

    // 法线：同为双精度 Normales: Ambas son de doble precision.
    glEnableClientState(GL_NORMAL_ARRAY);
    glNormalPointer(GL_DOUBLE, sizeof(Vertex),
        reinterpret_cast<const void*>(offsetof(Vertex, normal)));

    // 纹理坐标：float2（glm::vec2）Coordenadas de textura: float2(glm::vec2)
    glClientActiveTexture(GL_TEXTURE0);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex),
        reinterpret_cast<const void*>(offsetof(Vertex, texCoord)));

    // 解绑 Desatar
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (!indices.empty()) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    _isSetup = true;
}

void Mesh::draw() const {
    if (!_isSetup || VAO == 0) return;

    // 固定管线：有纹理才启用并绑定 Pipelines fijos: Se habilitan y enlazan solo cuando hay texturas presentes.
    if (textureID != 0) {
        glEnable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
    }

    // 绑定 VAO 并绘制 Vincular VAO y dibujar
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
}

unsigned int Mesh::getTextureID() const {
    return textureID; // 如果你的成员名不同，请改成对应名称
}

void Mesh::cleanup() {
    if (EBO) { glDeleteBuffers(1, &EBO); EBO = 0; }
    if (VBO) { glDeleteBuffers(1, &VBO); VBO = 0; }
    if (VAO) { glDeleteVertexArrays(1, &VAO); VAO = 0; }
}
