#define GLM_ENABLE_EXPERIMENTAL
#pragma once
#include "Transform.h"
#include <glm/gtc/quaternion.hpp>
#include <SDL3/SDL.h>

class Camera {
public:
    double fov = glm::radians(60.0);
    double aspect = 16.0 / 9.0;
    double zNear = 0.1;
    double zFar = 128.0;

    double moveSpeed = 6.0;
    double lookSensitivity = 0.0025;
    double panSpeed = 0.0025;
    double zoomSpeed = 1.0;

    Transform transform;

    mat4 projection() const;
    mat4 view() const;

    void onMouseButton(uint8_t button, int state, int x, int y);
    void onMouseMove(int x, int y);
    void onMouseWheel(int direction);
    void onKeyDown(int scancode);
    void onKeyUp(int scancode);
    void onSpecialKeyDown(int key) {}
    void onSpecialKeyUp(int key) {}

    void update(double dt);

    void focusOn(const vec3& targetCenter, double radius = 1.0);

private:
    // teclas
    bool _rmb = false, _lmb = false, _mmb = false;
    bool _alt = false, _shift = false;
    bool _w = false, _a = false, _s = false, _d = false, _q = false, _e = false;

    // mouse
    int _lastX = 0, _lastY = 0;
    bool _haveLast = false;

    // orientación/órbita
    double _yaw = 0.0;
    double _pitch = 0.0;
    vec3   _orbitTarget = vec3(0.0);
    double _orbitDistance = 5.0;

    void _applyYawPitchToBasis();
    void _moveFPS(double dt);
    static vec3 _worldUp() { return vec3(0, 1, 0); }
};
