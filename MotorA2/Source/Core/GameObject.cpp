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
    //if (isSelected) {
    //    glDisable(GL_LIGHTING);
    //    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //    glLineWidth(3.0f);
    //    glColor3f(1.0f, 1.0f, 0.0f);
    //    mesh->draw();
    //    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    //    glLineWidth(1.0f);
    //    glEnable(GL_LIGHTING);
    //}
    //if (isSelected) {
    //    glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT | GL_LINE_BIT);

    //    glDisable(GL_LIGHTING);
    //    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //    glLineWidth(3.0f);
    //    glColor3f(1.0f, 1.0f, 0.0f);
    //    mesh->draw();

    //    glPopAttrib();
    //}
    if (isSelected) {
        glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT | GL_LINE_BIT);

        glDisable(GL_LIGHTING);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(3.0f);
        glColor3f(1.0f, 1.0f, 0.0f);
        mesh->draw();

        glPopAttrib();
    }

    mesh->draw();
    glPopMatrix();
}

void GameObject::setMesh(std::shared_ptr<Mesh> m) {
    mesh = m;
    mesh->showVertexNormals = false;
    mesh->showFaceNormals = false;

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
//AABB GameObject::computeWorldAABB() const
//{
//    AABB result;
//    bool hasAny = false;
//
//    // 1️⃣ 自己的 mesh
//    if (mesh) {
//        // mesh 的 local AABB
//        //AABB local = mesh->aabb();   // ← 你 Mesh 里已经有这个
//        mat4 world = computeWorldMatrix(this);
//        AABB worldBox = mesh->getWorldAABB(world);
//
//        mat4 world = computeWorldMatrix(this);
//
//        // 把 local AABB 的 8 个角变换到世界空间
//        vec3 corners[8] = {
//            { local.min.x, local.min.y, local.min.z },
//            { local.max.x, local.min.y, local.min.z },
//            { local.min.x, local.max.y, local.min.z },
//            { local.max.x, local.max.y, local.min.z },
//            { local.min.x, local.min.y, local.max.z },
//            { local.max.x, local.min.y, local.max.z },
//            { local.min.x, local.max.y, local.max.z },
//            { local.max.x, local.max.y, local.max.z }
//        };
//
//        for (int i = 0; i < 8; ++i) {
//            vec4 w = world * vec4(corners[i], 1.0);
//            if (!hasAny) {
//                result.min = result.max = vec3(w);
//                hasAny = true;
//            }
//            else {
//                result.expand(vec3(w));
//            }
//        }
//    }
//
//    // 2️⃣ 子节点（递归）
//    for (auto* c : children) {
//        AABB childBox = c->computeWorldAABB();
//        if (!childBox.valid()) continue;
//
//        if (!hasAny) {
//            result = childBox;
//            hasAny = true;
//        }
//        else {
//            result.expand(childBox.min);
//            result.expand(childBox.max);
//        }
//    }
//
//    return result;
//}
AABB GameObject::computeWorldAABB() const
{
    auto isValid = [](const AABB& b) {
        return (b.min.x <= b.max.x) && (b.min.y <= b.max.y) && (b.min.z <= b.max.z);
        };

    AABB result;
    bool hasAny = false;

    // 1) 自己的 mesh
    if (mesh) {
        mat4 world = computeWorldMatrix(this);
        AABB worldBox = mesh->getWorldAABB(world);

        if (isValid(worldBox)) {
            result = worldBox;
            hasAny = true;
        }
    }

    // 2) 子节点递归合并
    for (auto* c : children) {
        AABB childBox = c->computeWorldAABB();
        if (!isValid(childBox)) continue;

        if (!hasAny) {
            result = childBox;
            hasAny = true;
        }
        else {
            result.expand(childBox.min);
            result.expand(childBox.max);
        }
    }

    return result;
}

