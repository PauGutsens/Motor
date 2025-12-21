#define GLM_ENABLE_EXPERIMENTAL
#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <cmath>

extern SDL_Window* window;

Camera::Camera() {
    // Posición Inicial:
    // X = 0 (Izq/Der)
    // Y = 5 (Altura)
    // Z = 20 (Atras/Adelante)
    transform.pos() = vec3(0.0, -50.0, 50.0);

    // Orientación Inicial:
    // Pitch para mirar hacia abajo
    _pitch = glm::radians(40.0);
    _yaw = 0.0; // Mirando hacia -Z

    // Inicializar variables de órbita
    // Es importante que la distancia coincida con tu posición Z inicial
    // para que el zoom funcione suave desde el principio.
    _orbitTarget = vec3(0.0f, 0.0f, 0.0f);
    _orbitDistance = 50.0;

    // Aplicar estos cambios a la matriz interna
    _applyYawPitchToBasis();
}

mat4 Camera::projection() const {
    return glm::perspective(fov, aspect, zNear, zFar);
}

mat4 Camera::view() const {
    // base Transform + normal
    vec3 f = glm::normalize(transform.fwd());
    vec3 u = glm::normalize(transform.up());
    vec3 p = transform.pos();
    return glm::lookAt(p, p + f, u);
}

void Camera::onMouseButton(uint8_t button, int state, int x, int y) {
    if (state == 1) { _lastX = x; _lastY = y; _haveLast = true; }

    if (button == SDL_BUTTON_RIGHT) {
        _rmb = (state == 1);
        // modo Fly
        SDL_SetWindowRelativeMouseMode(window, _rmb ? true : false);
    }
    else if (button == SDL_BUTTON_LEFT) {
        _lmb = (state == 1);
    }
    else if (button == SDL_BUTTON_MIDDLE) {
        _mmb = (state == 1);
    }
}

void Camera::onMouseMove(int x, int y) {
    if (!_haveLast) { _lastX = x; _lastY = y; _haveLast = true; return; }
    int dx = x - _lastX;
    int dy = y - _lastY;
    _lastX = x; _lastY = y;

    // Prioridad de modos
    if (_alt && _lmb)        _orbitPixels(dx, dy);
    else if (_mmb || (_rmb && _shift)) _panPixels(dx, dy);
    else if (_rmb)           _freeLookPixels(dx, dy);
}

void Camera::onMouseWheel(int direction) {
    _dollySteps(direction);
}

void Camera::onKeyDown(int scancode) {
    switch (scancode) {
    case SDL_SCANCODE_LALT: case SDL_SCANCODE_RALT:   _alt = true; break;
    case SDL_SCANCODE_LSHIFT: case SDL_SCANCODE_RSHIFT: _shift = true; break;
    case SDL_SCANCODE_W: _w = true; break;
    case SDL_SCANCODE_A: _a = true; break;
    case SDL_SCANCODE_S: _s = true; break;
    case SDL_SCANCODE_D: _d = true; break;
    case SDL_SCANCODE_Q: _q = true; break;
    case SDL_SCANCODE_E: _e = true; break;
    default: break;
    }
}

void Camera::onKeyUp(int scancode) {
    switch (scancode) {
    case SDL_SCANCODE_LALT: case SDL_SCANCODE_RALT:   _alt = false; break;
    case SDL_SCANCODE_LSHIFT: case SDL_SCANCODE_RSHIFT: _shift = false; break;
    case SDL_SCANCODE_W: _w = false; break;
    case SDL_SCANCODE_A: _a = false; break;
    case SDL_SCANCODE_S: _s = false; break;
    case SDL_SCANCODE_D: _d = false; break;
    case SDL_SCANCODE_Q: _q = false; break;
    case SDL_SCANCODE_E: _e = false; break;
    default: break;
    }
}

void Camera::update(double dt) {
    _moveFPS(dt);
    _applyYawPitchToBasis();
}

void Camera::focusOn(const vec3& targetCenter, double radius) {
    _orbitTarget = targetCenter;
    double d = std::max(0.001, 1.2 * radius / std::tan(fov * 0.5));
    _orbitDistance = d;

    vec3 f = glm::normalize(transform.fwd());
    vec3 newPos = _orbitTarget - f * _orbitDistance;
    transform.pos() = newPos;
}

void Camera::_applyYawPitchToBasis() {
    const double limit = glm::radians(89.0);
    _pitch = std::clamp(_pitch, -limit, limit);

    double cp = std::cos(_pitch), sp = std::sin(_pitch);
    double cy = std::cos(_yaw), sy = std::sin(_yaw);
    vec3 f = vec3(cp * sy, sp, -cp * cy);
    vec3 r = glm::normalize(glm::cross(f, _worldUp()));
    vec3 u = glm::normalize(glm::cross(r, f));
    vec3 l = -r;

    mat4& M = transform.mat_mutable();
    M[0] = vec4(l, 0.0);
    M[1] = vec4(u, 0.0);
    M[2] = vec4(f, 0.0);
    M[3] = vec4(transform.pos(), 1.0);
}

//void Camera::_freeLookPixels(int dx, int dy) {
//    _yaw -= dx * lookSensitivity;
//    _pitch -= dy * lookSensitivity;
//}
//
//void Camera::_orbitPixels(int dx, int dy) {
//    // órbita alrededor del tarjet
//    _yaw -= dx * lookSensitivity;
//    _pitch -= dy * lookSensitivity;
//    _applyYawPitchToBasis();
//
//    vec3 f = glm::normalize(transform.fwd());
//    transform.pos() = _orbitTarget - f * _orbitDistance;
//}
void Camera::_freeLookPixels(int dx, int dy) {
    _yaw += dx * lookSensitivity;
    _pitch += -dy * lookSensitivity; // 如果你觉得上下也反了，就用 -dy
}

void Camera::_orbitPixels(int dx, int dy) {
    _yaw += dx * lookSensitivity;
    _pitch += -dy * lookSensitivity;
    _applyYawPitchToBasis();

    vec3 f = glm::normalize(transform.fwd());
    transform.pos() = _orbitTarget - f * _orbitDistance;
}


void Camera::_panPixels(int dx, int dy) {
    vec3 r = glm::normalize(glm::cross(transform.fwd(), _worldUp()));
    vec3 u = glm::normalize(transform.up());

    double refDist = _orbitDistance;
    if (refDist <= 0.0) {
        refDist = 1.0;
    }
    double s = panSpeed * refDist;
    transform.pos() += (r * (double)dx + u * (double)dy) * s;
}

void Camera::_moveFPS(double dt) {
    if (!_rmb) return;

    double speed = moveSpeed * (_shift ? 2.0 : 1.0);
    vec3 f = glm::normalize(transform.fwd());
    vec3 r = glm::normalize(glm::cross(f, _worldUp()));
    vec3 move(0.0);

    if (_w) move += f;
    if (_s) move -= f;
    if (_d) move += r;  // D = derecha
    if (_a) move -= r;  // A = izquierda
    if (_q) move -= vec3(0, 1, 0);  // Q = bajar
    if (_e) move += vec3(0, 1, 0);  // E = subir

    if (glm::length(move) > 0.0) {
        move = glm::normalize(move) * (speed * dt);
        transform.pos() += move;
    }
}

void Camera::_dollySteps(int steps) {
    if (steps == 0) return;

    vec3 f = glm::normalize(transform.fwd());
    double ref = std::max(0.001, _orbitDistance);
    double delta = (double)steps * zoomSpeed * std::max(0.2, ref * 0.1);

    if (_alt || _lmb) {
        _orbitDistance = std::max(0.001, _orbitDistance - delta);
        transform.pos() = _orbitTarget - f * _orbitDistance;
    }
    else {
        transform.pos() += f * delta;
        _orbitDistance = glm::length(_orbitTarget - transform.pos());
    }
}
void Camera::setFromViewMatrix(const glm::mat4& view)
{
    // view 的逆 = 世界空间下的相机变换（world matrix）
    glm::mat4 invF = glm::inverse(view);

    // 你的 Transform 是 double（dmat4/dvec3），这里做一次 float->double
    mat4 inv = mat4(invF);

    // 相机位置：world matrix 第 4 列
    vec3 pos = vec3(inv[3]);

    // 从矩阵提取相机 forward/up：
    // 你的 view() 是 lookAt(p, p+f, u)，因此 world matrix 的第3列是 -forward
    vec3 forward = -glm::normalize(vec3(inv[2]));
    vec3 up = glm::normalize(vec3(inv[1]));
    vec3 right = glm::normalize(vec3(inv[0]));
    vec3 left = -right;

    // 1) 写回 Transform（位置 + basis）
    mat4& M = transform.mat_mutable();
    M[0] = vec4(left, 0.0);
    M[1] = vec4(up, 0.0);
    M[2] = vec4(forward, 0.0);
    M[3] = vec4(pos, 1.0);

    // 2) 同步 yaw/pitch（保持你原有的鼠标控制体系能继续用）
    // 你 _applyYawPitchToBasis() 里 forward 的公式是：
    // f = (cp*sy, sp, cp*cy)
    _pitch = std::asin(glm::clamp(forward.y, -1.0, 1.0));
    _yaw = std::atan2(forward.x, -forward.z);

    // 3) 重新应用一次（确保内部状态一致）
    _applyYawPitchToBasis();

    // 4) 同步 orbit 距离（可选但推荐：避免 Orbit 模式跳一下）
    _orbitDistance = glm::length(_orbitTarget - transform.pos());
}
void Camera::frameTo(const vec3& center, double radius)
{
    radius = std::max(radius, 0.01);
    _orbitTarget = center;

    // fov 在 Camera.h 里已经是弧度（glm::radians）
    const double margin = 1.2;
    _orbitDistance = std::max(0.001, margin * radius / std::tan(fov * 0.5));

    // 用当前 forward 放置相机（相机在 forward 的反方向）
    vec3 forward = glm::normalize(transform.fwd());
    transform.pos() = _orbitTarget - forward * _orbitDistance;
}
