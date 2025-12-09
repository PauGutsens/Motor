#include "GameObject.h"
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <vector>
#include <glm/gtc/matrix_inverse.hpp>

GameObject::GameObject(const std::string& n) : name(n) {
}

void GameObject::draw() const {
    if (!mesh) return;
    glPushMatrix();
    glMultMatrixd(glm::value_ptr(computeWorldMatrix(this)));
    if (isSelected) {
        glDisable(GL_LIGHTING);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(3.0f);
        glColor3f(1.0f, 1.0f, 0.0f);
        mesh->draw();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glLineWidth(1.0f);
        glEnable(GL_LIGHTING);
    }
    mesh->draw();
    glPopMatrix();
}

void GameObject::setMesh(std::shared_ptr<Mesh> m) {
    mesh = m;
}

void GameObject::setTexture(GLuint texID) {
    if (mesh) {
        mesh->setTexture(texID);
    }
}
unsigned int GameObject::getTextureID() const {
    if (!mesh) return 0;
    return mesh->getTextureID(); 
}

bool GameObject::isDescendantOf(const GameObject* p) const {
    for (auto cur = parent; cur; cur = cur->parent)
        if (cur == p) return true;
    return false;
}

int GameObject::indexInParent() const {
    if (!parent) return -1;
    const auto& v = parent->children;
    for (int i = 0; i < (int)v.size(); ++i)
        if (v[i] == this) return i;
    return -1;
}

void GameObject::addChild(GameObject* c, int index) {
    if (!c || c == this || this->isDescendantOf(c)) return;

    if (c->parent) {
        auto& from = c->parent->children;
        auto it = std::find(from.begin(), from.end(), c);
        if (it != from.end()) from.erase(it);
    }

    c->parent = this;
    if (index < 0 || index >(int)children.size()) {
        children.push_back(c);
    }
    else {
        children.insert(children.begin() + index, c);
    }
}

void GameObject::removeChild(GameObject* c) {
    if (!c) return;
    auto it = std::find(children.begin(), children.end(), c);
    if (it != children.end()) {
        children.erase(it);
        if (c->parent == this) c->parent = nullptr; // coherencia
    }
}

mat4 computeWorldMatrix(const GameObject* go) {
    mat4 M(1.0);
    std::vector<const GameObject*> chain;
    for (auto cur = go; cur; cur = cur->parent) chain.push_back(cur);
    for (auto it = chain.rbegin(); it != chain.rend(); ++it)
        M = M * (*it)->transform.mat();
    return M;
}

void setLocalFromWorld(GameObject* go, const mat4& M_world, const GameObject* newParent) {
    const mat4 M_parent = newParent ? computeWorldMatrix(newParent) : mat4(1.0);
    go->transform.mat_mutable() = glm::inverse(M_parent) * M_world;
}