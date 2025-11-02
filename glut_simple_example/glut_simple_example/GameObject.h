#pragma once

#include "Transform.h"
#include "Mesh.h"
#include <memory>
#include <string>

class GameObject {
public:
    std::string name;
    Transform transform;
    std::shared_ptr<Mesh> mesh;
    bool isSelected = false;

    GameObject(const std::string& n = "GameObject");

    void draw() const;
    void setMesh(std::shared_ptr<Mesh> m);
    void setTexture(GLuint texID);
    unsigned int getTextureID() const;

};