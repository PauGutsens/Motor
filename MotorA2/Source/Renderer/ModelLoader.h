#pragma once
#include "Mesh.h"
#include <string>
#include <vector>
#include <memory>

class ModelLoader {
public:
    static std::vector<std::shared_ptr<Mesh>> loadModel(const std::string& path);
    static GLuint loadTexture(const std::string& path);

private:
    static std::shared_ptr<Mesh> processMesh(void* mesh, const void* scene);
};