#pragma once
#include "Transform.h"
#include "Mesh.h"
#include <memory>
#include <string>
#include <vector>

class GameObject {
public:
    std::string name;
    Transform transform;
    std::shared_ptr<Mesh> mesh;
    bool isSelected = false;

    GameObject* parent = nullptr;
    std::vector<GameObject*> children;

    GameObject(const std::string& n = "GameObject");
    void draw() const;
    void setMesh(std::shared_ptr<Mesh> m);
    void setTexture(GLuint texID);
    unsigned int getTextureID() const;

    bool isDescendantOf(const GameObject* p) const;
    int indexInParent() const;
    void addChild(GameObject* c, int index = -1);
    void removeChild(GameObject* c);
    AABB computeWorldAABB() const;

};

mat4 computeWorldMatrix(const GameObject* go);
void setLocalFromWorld(GameObject* go, const mat4& M_world, const GameObject* newParent);
