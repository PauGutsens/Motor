#pragma once
#include "types.h"
#include "AABB.h"
#include <cmath>

struct Plane
{
    // Plane equation: dot(n, x) + d >= 0 means "inside"
    vec3 n = vec3(0.0);
    double d = 0.0;

    void normalize()
    {
        double len = glm::length(n);
        if (len > 1e-12)
        {
            n /= len;
            d /= len;
        }
    }

    double distance(const vec3& p) const
    {
        return glm::dot(n, p) + d;
    }
};

struct Frustum
{
    // Order: left, right, bottom, top, near, far
    Plane p[6];

    // Build from View-Projection matrix (OpenGL style: clip = VP * worldPos)
    static Frustum fromViewProjection(const mat4& VP)
    {
        // GLM is column-major; easiest is to use rows via transpose.
        mat4 T = glm::transpose(VP);
        vec4 r0 = T[0];
        vec4 r1 = T[1];
        vec4 r2 = T[2];
        vec4 r3 = T[3];

        Frustum f;

        auto makePlane = [](const vec4& v) -> Plane
            {
                Plane pl;
                pl.n = vec3(v.x, v.y, v.z);
                pl.d = (double)v.w;
                pl.normalize();
                return pl;
            };

        // Classic extraction:
        f.p[0] = makePlane(r3 + r0); // left
        f.p[1] = makePlane(r3 - r0); // right
        f.p[2] = makePlane(r3 + r1); // bottom
        f.p[3] = makePlane(r3 - r1); // top
        f.p[4] = makePlane(r3 + r2); // near
        f.p[5] = makePlane(r3 - r2); // far

        return f;
    }

    // AABB vs frustum using "positive vertex" method.
    // If the most positive vertex is outside any plane => completely outside.
    bool intersects(const AABB& b) const
    {
        if (!b.isValid()) return true; // if invalid, don't cull (safe)

        for (int i = 0; i < 6; ++i)
        {
            const Plane& pl = p[i];

            // pick the vertex most in direction of plane normal
            vec3 v;
            v.x = (pl.n.x >= 0.0) ? b.max.x : b.min.x;
            v.y = (pl.n.y >= 0.0) ? b.max.y : b.min.y;
            v.z = (pl.n.z >= 0.0) ? b.max.z : b.min.z;

            if (pl.distance(v) < 0.0)
                return false; // outside
        }
        return true; // inside or intersecting
    }
};
