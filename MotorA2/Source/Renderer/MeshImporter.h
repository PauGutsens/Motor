#pragma once
#include "Mesh.h"
#include <vector>
#include <string>
#include <memory>

// Estructura para la cabecera del archivo
// Nos dice cuantos vertices e indices tiene el archivo
struct MeshHeader {
    unsigned int numVertices;
    unsigned int numIndices;
};

class MeshImporter {
public:
    // Guarda la malla en un archivo propio (.myMesh)
    static bool Save(const char* path, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);

    // Carga la malla desde nuestro archivo propio
    static std::shared_ptr<Mesh> Load(const char* path);
};