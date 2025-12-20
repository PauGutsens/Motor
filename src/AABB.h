#pragma once
#include "types.h"
#include <limits>
#include <algorithm>

struct AABB {
    vec3 min;
    vec3 max;

    AABB()
        : min(vec3(std::numeric_limits<double>::max()))
        , max(vec3(std::numeric_limits<double>::lowest()))
    {
    }

    AABB(const vec3& minPoint, const vec3& maxPoint)
        : min(minPoint), max(maxPoint)
    {
    }

    void expand(const vec3& point) {
        min.x = std::min(min.x, point.x);
        min.y = std::min(min.y, point.y);
        min.z = std::min(min.z, point.z);
        max.x = std::max(max.x, point.x);
        max.y = std::max(max.y, point.y);
        max.z = std::max(max.z, point.z);
    }

    void merge(const AABB& other) {
        if (!other.isValid()) return;
        expand(other.min);
        expand(other.max);
    }

    vec3 center() const {
        return (min + max) * 0.5;
    }

    vec3 size() const {
        return max - min;
    }

    AABB transform(const mat4& matrix) const {
        AABB result;
        vec3 corners[8] = {
            vec3(min.x, min.y, min.z),
            vec3(max.x, min.y, min.z),
            vec3(min.x, max.y, min.z),
            vec3(max.x, max.y, min.z),
            vec3(min.x, min.y, max.z),
            vec3(max.x, min.y, max.z),
            vec3(min.x, max.y, max.z),
            vec3(max.x, max.y, max.z)
        };

        for (int i = 0; i < 8; ++i) {
            vec4 transformed = matrix * vec4(corners[i], 1.0);
            result.expand(vec3(transformed));
        }

        return result;
    }

    bool isValid() const {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    bool intersects(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }
};

struct Ray {
    vec3 origin;
    vec3 direction;

    Ray() : origin(0.0), direction(0.0, 0.0, 1.0) {}
    Ray(const vec3& orig, const vec3& dir)
        : origin(orig), direction(glm::normalize(dir))
    {
    }
    vec3 pointAt(double t) const {
        return origin + direction * t;
    }
    bool intersectsAABB(const AABB& box, double& t) const {
        double tmin = (box.min.x - origin.x) / direction.x;
        double tmax = (box.max.x - origin.x) / direction.x;
        if (tmin > tmax) std::swap(tmin, tmax);
        double tymin = (box.min.y - origin.y) / direction.y;
        double tymax = (box.max.y - origin.y) / direction.y;
        if (tymin > tymax) std::swap(tymin, tymax);
        if ((tmin > tymax) || (tymin > tmax))
            return false;
        if (tymin > tmin)
            tmin = tymin;
        if (tymax < tmax)
            tmax = tymax;
        double tzmin = (box.min.z - origin.z) / direction.z;
        double tzmax = (box.max.z - origin.z) / direction.z;
        if (tzmin > tzmax) std::swap(tzmin, tzmax);
        if ((tmin > tzmax) || (tzmin > tmax))
            return false;
        if (tzmin > tmin)
            tmin = tzmin;
        if (tzmax < tmax)
            tmax = tzmax;
        t = tmin;
        return tmin >= 0.0;
    }
};