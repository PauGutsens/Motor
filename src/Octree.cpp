#include "Octree.h"
#include "Frustum.h" // [NEW] Needed for incomplete type in header
#include <algorithm>
#include <iostream>

/*-------------------------------------------------------------------------
  OctreeNode Implementation
-------------------------------------------------------------------------*/

// Constantes de configuración
static int MAX_OBJECTS = 8;
static int MAX_DEPTH = 6;

OctreeNode::OctreeNode(const AABB& bounds, int d) : box(bounds), depth(d) {
    for (int i = 0; i < 8; ++i) children[i] = nullptr;
}

void OctreeNode::split() {
    isLeaf = false;
    vec3 center = box.center();
    vec3 min = box.min;
    vec3 max = box.max;

    // Generar los 8 sub-nodos
    // Hijos inferiores
    children[0] = std::make_unique<OctreeNode>(AABB(min, center), depth + 1); // 000
    children[1] = std::make_unique<OctreeNode>(AABB(vec3(center.x, min.y, min.z), vec3(max.x, center.y, center.z)), depth + 1); // 100
    children[2] = std::make_unique<OctreeNode>(AABB(vec3(min.x, min.y, center.z), vec3(center.x, center.y, max.z)), depth + 1); // 001
    children[3] = std::make_unique<OctreeNode>(AABB(vec3(center.x, min.y, center.z), vec3(max.x, center.y, max.z)), depth + 1); // 101

    // Hijos superiores
    children[4] = std::make_unique<OctreeNode>(AABB(vec3(min.x, center.y, min.z), vec3(center.x, max.y, center.z)), depth + 1); // 010
    children[5] = std::make_unique<OctreeNode>(AABB(vec3(center.x, center.y, min.z), vec3(max.x, max.y, center.z)), depth + 1); // 110
    children[6] = std::make_unique<OctreeNode>(AABB(vec3(min.x, center.y, center.z), vec3(center.x, max.y, max.z)), depth + 1); // 011
    children[7] = std::make_unique<OctreeNode>(AABB(center, max), depth + 1); // 111
}

bool OctreeNode::insert(std::shared_ptr<GameObject> go) {
    if (!go->mesh) return false;
    mat4 worldMatrix = computeWorldMatrix(go.get());
    AABB objBox = go->mesh->getWorldAABB(worldMatrix);

    if (!box.intersects(objBox)) return false; // El objeto no toca este nodo

    // Si es hoja y hay espacio, o nivel máx, insertar aquí
    if (isLeaf) {
        if (objects.size() < MAX_OBJECTS || depth >= MAX_DEPTH) {
            objects.push_back(go);
            return true;
        }
        else {
            split();
            // Mover objetos existentes a los hijos
            while (!objects.empty()) {
                auto obj = objects.back();
                objects.pop_back();
                bool insertedInChild = false;
                for (int i = 0; i < 8; ++i) {
                    if (children[i]->insert(obj)) {
                        insertedInChild = true;
                        // Nota: Un objeto puede estar en múltiples nodos si intersecta
                        // pero para simplificar, a menudo se intenta poner solo en 
                        // el nodo más pequeño que lo contenga. 
                        // PERO, la lógica simple de Octree es: si cabe, va al hijo.
                        // Si el objeto es grande, podría tener que quedarse en el padre.
                        // IMPLEMENTACION SIMPLE:
                        // Si intersecta con un hijo, lo metemos en ese hijo. 
                        // Si intersecta con varios, se duplica la referencia (no el objeto).
                    }
                }
            }
        }
    }
    
    // Si no es hoja, intentar meter en hijos
    if (!isLeaf) {
        bool inChild = false;
        for (int i = 0; i < 8; ++i) {
            if (children[i]->insert(go)) inChild = true;
        }
        return inChild;
    }

    return false; // Should not reach here
}

void OctreeNode::collectIntersections(const Frustum& frustum, std::list<std::shared_ptr<GameObject>>& results) const {
    if (!frustum.containsAABB(box)) return;

    for (const auto& obj : objects) {
        results.push_back(obj);
    }
    if (!isLeaf) {
        for (int i = 0; i < 8; ++i) {
            children[i]->collectIntersections(frustum, results);
        }
    }
}

void OctreeNode::collectIntersections(const Ray& ray, std::list<std::shared_ptr<GameObject>>& results) const {
    double t;
    if (!ray.intersectsAABB(box, t)) return;

    for (const auto& obj : objects) {
        results.push_back(obj);
    }

    if (!isLeaf) {
        // Ordenar hijos por distancia podría optimizar, pero overkill por ahora
        for (int i = 0; i < 8; ++i) {
            children[i]->collectIntersections(ray, results);
        }
    }
}

void OctreeNode::drawDebug() const {
    // Dibujar caja del nodo
    glDisable(GL_LIGHTING);
    glColor3f(0.0f, 1.0f, 1.0f); // Cyan para nodos
    glLineWidth(1.0f);
    
    vec3 min = box.min;
    vec3 max = box.max;
    
    glBegin(GL_LINES);
    glVertex3d(min.x, min.y, min.z); glVertex3d(max.x, min.y, min.z);
    glVertex3d(max.x, min.y, min.z); glVertex3d(max.x, min.y, max.z);
    glVertex3d(max.x, min.y, max.z); glVertex3d(min.x, min.y, max.z);
    glVertex3d(min.x, min.y, max.z); glVertex3d(min.x, min.y, min.z);

    glVertex3d(min.x, max.y, min.z); glVertex3d(max.x, max.y, min.z);
    glVertex3d(max.x, max.y, min.z); glVertex3d(max.x, max.y, max.z);
    glVertex3d(max.x, max.y, max.z); glVertex3d(min.x, max.y, max.z);
    glVertex3d(min.x, max.y, max.z); glVertex3d(min.x, max.y, min.z);

    glVertex3d(min.x, min.y, min.z); glVertex3d(min.x, max.y, min.z);
    glVertex3d(max.x, min.y, min.z); glVertex3d(max.x, max.y, min.z);
    glVertex3d(max.x, min.y, max.z); glVertex3d(max.x, max.y, max.z);
    glVertex3d(min.x, min.y, max.z); glVertex3d(min.x, max.y, max.z);
    glEnd();
    glEnable(GL_LIGHTING);

    if (!isLeaf) {
        for (int i = 0; i < 8; ++i) {
            children[i]->drawDebug();
        }
    }
}


/*-------------------------------------------------------------------------
  Octree Implementation
-------------------------------------------------------------------------*/

Octree::Octree(const AABB& rootRegion, int maxObj, int maxD) 
    : rootBounds(rootRegion), maxObjects(maxObj), maxDepthLevel(maxD) 
{
    MAX_OBJECTS = maxObj;
    MAX_DEPTH = maxD;
    clear();
}

void Octree::clear() {
    root = std::make_unique<OctreeNode>(rootBounds, 0);
}

void Octree::insert(std::shared_ptr<GameObject> go) {
    if (!root) return;
    // Si el objeto está fuera de los límites mundiales, podríamos expandir, 
    // pero por ahora lo ignoramos o lo forzamos.
    root->insert(go);
}

std::list<std::shared_ptr<GameObject>> Octree::queryFrustum(const Frustum& frustum) const {
    std::list<std::shared_ptr<GameObject>> results;
    if (root) {
        root->collectIntersections(frustum, results);
        // Eliminar duplicados (porque un objeto puede estar en varios nodos)
        results.sort();
        results.unique();
    }
    return results;
}

std::list<std::shared_ptr<GameObject>> Octree::queryRay(const Ray& ray) const {
    std::list<std::shared_ptr<GameObject>> results;
    if (root) {
        root->collectIntersections(ray, results);
        results.sort();
        results.unique();
    }
    return results;
}

void Octree::drawDebug() const {
    if (root) root->drawDebug();
}
