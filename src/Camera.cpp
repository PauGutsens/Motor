#define GLM_ENABLE_EXPERIMENTAL
#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <cmath>

extern SDL_Window* window;

mat4 Camera::projection() const {
return glm::perspective(fov, aspect, zNear, zFar);
}

mat4 Camera::view() const {
    vec3 f = glm::normalize(transform.fwd());
    vec3 u = glm::normalize(transform.up());
    vec3 p = transform.pos();
    return glm::lookAt(p, p + f, u);
}

void Camera::onMouseButton(uint8_t button, int state, int x, int y) {
 if (state == 1) { _lastX = x; _lastY = y; _haveLast = true; }

    if (button == SDL_BUTTON_RIGHT) {
        _rmb = (state == 1);
        // Usar relative mouse mode para permitir rotaciones sin límites
        SDL_SetWindowRelativeMouseMode(window, _rmb ? true : false);
    }
}

void Camera::onMouseMove(int x, int y) {
    if (!_haveLast) { _lastX = x; _lastY = y; _haveLast = true; return; }
    if (!_rmb) return;  // Solo procesar si RMB está presionado
    
    int dx = x - _lastX;
    int dy = y - _lastY;
    _lastX = x; 
    _lastY = y;

    // Rotación (Yaw/Pitch) - como Unity
    // Con relative mouse mode, los valores de x,y ya son relativos
    _yaw -= dx * lookSensitivity;
    _pitch -= dy * lookSensitivity;
    
    // Aplicar rotación inmediatamente
    _applyYawPitchToBasis();
}

void Camera::onMouseWheel(int direction) {
 if (direction == 0) return;
    
    // Zoom hacia adelante/atrás (como Unity)
    vec3 f = glm::normalize(transform.fwd());
    double delta = (double)direction * zoomSpeed;
    transform.pos() += f * delta;
}

void Camera::onKeyDown(int scancode) {
    switch (scancode) {
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
    _applyYawPitchToBasis();
    _moveFPS(dt);
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
    
    // Construir vector forward a partir de yaw/pitch
    vec3 f = vec3(cp * sy, sp, cp * cy);
  
    // Recalcular Right y Up
    vec3 r = glm::normalize(glm::cross(f, _worldUp()));
    vec3 u = glm::normalize(glm::cross(r, f));
    vec3 l = -r;

    mat4& M = transform.mat_mutable();
    M[0] = vec4(l, 0.0);
    M[1] = vec4(u, 0.0);
    M[2] = vec4(f, 0.0);
    M[3] = vec4(transform.pos(), 1.0);
}

void Camera::_moveFPS(double dt) {
    if (!_rmb) return;

  // Movimiento solo si RMB está presionado
    double speed = moveSpeed * dt;
    vec3 f = glm::normalize(transform.fwd());
    vec3 r = glm::normalize(transform.right());
    vec3 u = vec3(0, 1, 0);  // Siempre arriba en world space
    
    vec3 move(0.0);

    if (_w) move += f;      // Forward (along camera forward)
    if (_s) move -= f;      // Backward (opposite to forward)
    if (_d) move += r;      // Right (along camera right)
    if (_a) move -= r;      // Left (opposite to right)
    if (_e) move += u;      // Up (world up)
    if (_q) move -= u;      // Down (world down)

    if (glm::length(move) > 0.0001) {
        move = glm::normalize(move) * speed;
        transform.pos() += move;
    }
}
