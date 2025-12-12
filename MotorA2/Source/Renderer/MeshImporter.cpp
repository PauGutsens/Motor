#include "MeshImporter.h"
#include <fstream>
#include <iostream>

bool MeshImporter::Save(const char* path, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
    // Abrir el archivo para escribir en binario
    std::ofstream file(path, std::ios::out | std::ios::binary);
    if (!file.is_open()) return false;

    // Preparar la cabecera con los tamaños
    MeshHeader header;
    header.numVertices = vertices.size();
    header.numIndices = indices.size();

    // Escribir la cabecera
    file.write((char*)&header, sizeof(MeshHeader));

    // Escribir todos los vertices de golpe
    file.write((char*)vertices.data(), sizeof(Vertex) * vertices.size());

    // Escribir todos los indices de golpe
    file.write((char*)indices.data(), sizeof(unsigned int) * indices.size());

    file.close();
    return true;
}

std::shared_ptr<Mesh> MeshImporter::Load(const char* path) {
    // Abrir el archivo para leer en binario
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) return nullptr;

    // Leer la cabecera primero
    MeshHeader header;
    file.read((char*)&header, sizeof(MeshHeader));

    // Reservar memoria para los datos
    std::vector<Vertex> vertices(header.numVertices);
    std::vector<unsigned int> indices(header.numIndices);

    // Leer los vertices del archivo
    file.read((char*)vertices.data(), sizeof(Vertex) * header.numVertices);

    // Leer los indices del archivo
    file.read((char*)indices.data(), sizeof(unsigned int) * header.numIndices);

    file.close();

    // Crear la malla con los datos leidos
    auto mesh = std::make_shared<Mesh>(vertices, indices);
    mesh->setupMesh(); // Subir a la VRAM

    return mesh;
}