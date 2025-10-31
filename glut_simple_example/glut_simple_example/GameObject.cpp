#include "GameObject.h"
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

GameObject::GameObject(const std::string& n) : name(n) {
}

void GameObject::draw() const {
    if (!mesh) return;

    glPushMatrix();
    glMultMatrixd(glm::value_ptr(transform.mat()));

    // Draw selection outline if selected
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