#pragma once
#include "types.h"
#include "AABB.h"
#include <glm/glm.hpp>

struct Plane {
    vec3 normal;   
    double distance;

    Plane() : normal(0, 1, 0), distance(0) {}

    Plane(const vec3& n, double d)
        : normal(glm::normalize(n)), distance(d)
    {
    }

    Plane(const vec3& p1, const vec3& p2, const vec3& p3) {
        vec3 v1 = p2 - p1;
        vec3 v2 = p3 - p1;
        normal = glm::normalize(glm::cross(v1, v2));
        distance = -glm::dot(normal, p1);
    }

    double distanceToPoint(const vec3& point) const {
        return glm::dot(normal, point) + distance;
    }

    void normalize() {
        double length = glm::length(normal);
        if (length > 0.0) {
            normal /= length;
            distance /= length;
        }
    }
};

class Frustum {
public:
    enum Side {
        LEFT = 0,
        RIGHT,
        BOTTOM,
        TOP,
        NEAR,
        FAR,
        COUNT
    };

    Plane planes[COUNT];

    Frustum() {}

    void extractFromCamera(const mat4& projView) {

        // Left plane
        planes[LEFT].normal.x = projView[0][3] + projView[0][0];
        planes[LEFT].normal.y = projView[1][3] + projView[1][0];
        planes[LEFT].normal.z = projView[2][3] + projView[2][0];
        planes[LEFT].distance = projView[3][3] + projView[3][0];

        // Right plane
        planes[RIGHT].normal.x = projView[0][3] - projView[0][0];
        planes[RIGHT].normal.y = projView[1][3] - projView[1][0];
        planes[RIGHT].normal.z = projView[2][3] - projView[2][0];
        planes[RIGHT].distance = projView[3][3] - projView[3][0];

        // Bottom plane
        planes[BOTTOM].normal.x = projView[0][3] + projView[0][1];
        planes[BOTTOM].normal.y = projView[1][3] + projView[1][1];
        planes[BOTTOM].normal.z = projView[2][3] + projView[2][1];
        planes[BOTTOM].distance = projView[3][3] + projView[3][1];

        // Top plane
        planes[TOP].normal.x = projView[0][3] - projView[0][1];
        planes[TOP].normal.y = projView[1][3] - projView[1][1];
        planes[TOP].normal.z = projView[2][3] - projView[2][1];
        planes[TOP].distance = projView[3][3] - projView[3][1];

        // Near plane
        planes[NEAR].normal.x = projView[0][3] + projView[0][2];
        planes[NEAR].normal.y = projView[1][3] + projView[1][2];
        planes[NEAR].normal.z = projView[2][3] + projView[2][2];
        planes[NEAR].distance = projView[3][3] + projView[3][2];

        // Far plane
        planes[FAR].normal.x = projView[0][3] - projView[0][2];
        planes[FAR].normal.y = projView[1][3] - projView[1][2];
        planes[FAR].normal.z = projView[2][3] - projView[2][2];
        planes[FAR].distance = projView[3][3] - projView[3][2];

        for (int i = 0; i < COUNT; i++) {
            planes[i].normalize();
        }
    }

    bool containsAABB(const AABB& box) const {
        for (int i = 0; i < COUNT; i++) {
            vec3 positiveVertex = box.min;
            if (planes[i].normal.x >= 0) positiveVertex.x = box.max.x;
            if (planes[i].normal.y >= 0) positiveVertex.y = box.max.y;
            if (planes[i].normal.z >= 0) positiveVertex.z = box.max.z;
            if (planes[i].distanceToPoint(positiveVertex) < 0) {
                return false; 
            }
        }
        return true;
    }

    void getCorners(vec3 corners[8], const mat4& invProjView) const {
        vec4 ndcCorners[8] = {
            vec4(-1, -1, -1, 1), vec4(1, -1, -1, 1),
            vec4(-1,  1, -1, 1), vec4(1,  1, -1, 1),
            vec4(-1, -1,  1, 1), vec4(1, -1,  1, 1),
            vec4(-1,  1,  1, 1), vec4(1,  1,  1, 1)
        };

        // Transformar a world space
        for (int i = 0; i < 8; i++) {
            vec4 worldPos = invProjView * ndcCorners[i];
            worldPos /= worldPos.w;
            corners[i] = vec3(worldPos);
        }
    }
};