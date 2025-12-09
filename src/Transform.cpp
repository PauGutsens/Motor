#include "Transform.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

void Transform::translate(const vec3& v) { _mat = glm::translate(_mat, v); }
void Transform::rotate(double rads, const vec3& v) { _mat = glm::rotate(_mat, rads, v); }
void Transform::setPosition(const vec3& t) {
    _pos = t;
    _pos_w = (mat4::value_type)1;
}

static inline mat4::value_type _len3(const vec3& v) {
    return (mat4::value_type)std::sqrt((double)v.x * (double)v.x +
        (double)v.y * (double)v.y +
        (double)v.z * (double)v.z);
}

vec3 Transform::getScale() const {
    return vec3(_len3(_left), _len3(_up), _len3(_fwd));
}

void Transform::setScale(const vec3& s) {
    const mat4::value_type eps = (mat4::value_type)1e-12;
    auto norm_safe = [&](const vec3& v, int axis)->vec3 {
        auto L = _len3(v);
        if (L > eps) return v * (mat4::value_type)(1.0 / (double)L);
        if (axis == 0) return vec3(1, 0, 0);
        if (axis == 1) return vec3(0, 1, 0);
        return vec3(0, 0, 1);
        };
    vec3 nx = norm_safe(_left, 0);
    vec3 ny = norm_safe(_up, 1);
    vec3 nz = norm_safe(_fwd, 2);
    _left = nx * s.x;  _left_w = (mat4::value_type)0;
    _up = ny * s.y;  _up_w = (mat4::value_type)0;
    _fwd = nz * s.z;  _fwd_w = (mat4::value_type)0;
    _pos_w = (mat4::value_type)1;
}

void Transform::resetScale() {
    setScale(vec3(1, 1, 1));
}

void Transform::rotateEulerDeltaDeg(const vec3& degXYZ) {
    const mat4::value_type k = (mat4::value_type)(3.14159265358979323846 / 180.0);
    if (degXYZ.x != (mat4::value_type)0) rotate(degXYZ.x * k, vec3(1, 0, 0));
    if (degXYZ.y != (mat4::value_type)0) rotate(degXYZ.y * k, vec3(0, 1, 0));
    if (degXYZ.z != (mat4::value_type)0) rotate(degXYZ.z * k, vec3(0, 0, 1));
}

void Transform::resetRotation() {
    vec3 T = _pos;
    vec3 S = getScale();
    _left = vec3(S.x, 0, 0);  _left_w = (mat4::value_type)0;
    _up = vec3(0, S.y, 0);  _up_w = (mat4::value_type)0;
    _fwd = vec3(0, 0, S.z);  _fwd_w = (mat4::value_type)0;
    _pos = T;                
    _pos_w = (mat4::value_type)1;
}