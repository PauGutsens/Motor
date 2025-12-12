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

    std::string modelPath; // For simple serialization
    int meshIndex = -1;    // For simple serialization

    struct CameraComponent {
        bool enabled = false;
        double fov = 1.047; // ~60 degrees in radians
        double zNear = 0.1;
        double zFar = 1000.0;
        double aspect = 16.0 / 9.0;
    };
    CameraComponent camera;

    GameObject(const std::string& n = "GameObject");
    void draw() const;
    void setMesh(std::shared_ptr<Mesh> m);
    void setTexture(GLuint texID);
    unsigned int getTextureID() const;

    bool isDescendantOf(const GameObject* p) const;
    int indexInParent() const;
    void addChild(GameObject* c, int index = -1);
    void removeChild(GameObject* c);
};

mat4 computeWorldMatrix(const GameObject* go);
void setLocalFromWorld(GameObject* go, const mat4& M_world, const GameObject* newParent);
