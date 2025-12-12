#pragma once
#include "AABB.h"
#include "GameObject.h"
#include <vector>
#include <memory>
#include <list>
#include <list>
#include <GL/glew.h>

class Frustum; // Forward declaration

class OctreeNode {
public:
    AABB box;
    std::vector<std::shared_ptr<GameObject>> objects; // Objetos en este nodo
    std::unique_ptr<OctreeNode> children[8];
    bool isLeaf = true;
    int depth = 0;

    OctreeNode(const AABB& bounds, int d);
    
    void split();
    bool insert(std::shared_ptr<GameObject> go);
    void collectIntersections(const Frustum& frustum, std::list<std::shared_ptr<GameObject>>& results) const;
    void collectIntersections(const Ray& ray, std::list<std::shared_ptr<GameObject>>& results) const;
    void drawDebug() const;
};

class Octree {
public:
    Octree(const AABB& rootRegion, int maxObjectsPerNode = 8, int maxDepth = 6);
    
    void clear();
    void insert(std::shared_ptr<GameObject> go);
    
    // Devuelve lista de objetos candidatos
    std::list<std::shared_ptr<GameObject>> queryFrustum(const Frustum& frustum) const;
    std::list<std::shared_ptr<GameObject>> queryRay(const Ray& ray) const;

    void drawDebug() const;

private:
    std::unique_ptr<OctreeNode> root;
    AABB rootBounds;
    int maxObjects;
    int maxDepthLevel;
};
