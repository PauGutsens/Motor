#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <SDL3/SDL.h>
#include <algorithm>

static SDL_Window* GetCurrentWindow() 
{
    extern SDL_Window* window;
    return window;
}

glm::dmat4 Camera::projection() const 
{
    return glm::perspective(fov, aspect, zNear, zFar);
}

glm::dmat4 Camera::view() const 
{
    return glm::lookAt(_transform.pos(), _transform.pos() + _transform.fwd(), _transform.up());
}

void Camera::onMouseButton(int button, int state, int x, int y) 
{
    if (button == SDL_BUTTON_RIGHT) {
        _rightMouseDown = (state == 1);
        if (_rightMouseDown) {
             _lastMouseX = x;
            _lastMouseY = y;
            SDL_CaptureMouse(true);
        }
        else {SDL_CaptureMouse(false); }

    }
    else if (button == SDL_BUTTON_MIDDLE) {     // ⭐ 新增：中键按下
        _middleMouseDown = (state == 1);
        if (_middleMouseDown) { _lastMouseX = x; _lastMouseY = y; }
        
    }
    else if (button == SDL_BUTTON_LEFT) {
        _leftMouseDown = (state == 1);

        if (_leftMouseDown) {_lastMouseX = x; _lastMouseY = y;}
    }
}

void Camera::onMouseMove(int x, int y) 
{
    int deltaX = x - _lastMouseX;
    int deltaY = y - _lastMouseY;

    if (_rightMouseDown && !_altPressed) 
    {
        _yaw -= deltaX * lookSensitivity;
        _pitch -= deltaY * lookSensitivity;

        const double maxPitch = glm::radians(89.0);
        _pitch = glm::clamp(_pitch, -maxPitch, maxPitch);

        updateOrientation();

        _lastMouseX = x;
        _lastMouseY = y;
    }
    if ((_rightMouseDown && _shiftPressed) || _middleMouseDown) {
        handlePan(deltaX, deltaY);
        _lastMouseX = x;
        _lastMouseY = y;
        
    }
     
       else if (_rightMouseDown && !_altPressed)
    {
        handleOrbit(deltaX, deltaY);
        _lastMouseX = x;
        _lastMouseY = y;
    }
    else {
        _lastMouseX = x;
        _lastMouseY = y;
    }
}

void Camera::onMouseWheel(int direction) 
{
    double zoomAmount = direction * zoomSpeed;
    _transform.translate(vec3(0, 0, -zoomAmount));
}

void Camera::onKeyDown(unsigned char key) 
{
    switch (key) {
    case SDL_SCANCODE_W: _keyW = true; break;
    case SDL_SCANCODE_A: _keyA = true; break;
    case SDL_SCANCODE_S: _keyS = true; break;
    case SDL_SCANCODE_D: _keyD = true; break;
    case SDL_SCANCODE_F: focusOn(orbitTarget, orbitDistance); break;
    }
}

void Camera::onKeyUp(unsigned char key) 
{
    switch (key) {
    case SDL_SCANCODE_W: _keyW = false; break;
    case SDL_SCANCODE_A: _keyA = false; break;
    case SDL_SCANCODE_S: _keyS = false; break;
    case SDL_SCANCODE_D: _keyD = false; break;
    }
}

void Camera::onSpecialKeyDown(int key) 
{
    switch (key) {
    case SDL_SCANCODE_LSHIFT:
    case SDL_SCANCODE_RSHIFT:
        _shiftPressed = true;
 break;
    case SDL_SCANCODE_LALT:
    case SDL_SCANCODE_RALT:
_altPressed = true;
    break;
}
}

void Camera::onSpecialKeyUp(int key) 
{
    switch (key) {
    case SDL_SCANCODE_LSHIFT:
    case SDL_SCANCODE_RSHIFT:
     _shiftPressed = false;
        break;
    case SDL_SCANCODE_LALT:
    case SDL_SCANCODE_RALT:
   _altPressed = false;
   break;
    }
}

void Camera::update(double deltaTime) 
{
    if (_rightMouseDown) {
      handleFPSMovement(deltaTime);
    }
}

void Camera::focusOn(const vec3& target, double distance) 
{
    orbitTarget = target;
    orbitDistance = distance;

vec3 offset = _transform.fwd() * distance;
_transform.pos() = target - offset;
}

void Camera::updateOrientation() 
{
    mat4 rotation = mat4(1.0);
    rotation = glm::rotate(rotation, _yaw, vec3(0, 1, 0));
    rotation = glm::rotate(rotation, _pitch, vec3(1, 0, 0));

    vec3 forward = vec3(rotation * vec4(0, 0, -1, 0));
    vec3 right = glm::normalize(glm::cross(forward, vec3(0, 1, 0)));
    vec3 up = glm::normalize(glm::cross(right, forward));

    vec3 currentPos = _transform.pos();

    auto& m = _transform.mat_mutable();
    m[0] = vec4(right, 0);
    m[1] = vec4(up, 0);
    m[2] = vec4(-forward, 0);
    m[3] = vec4(currentPos, 1);
}

void Camera::handleFPSMovement(double deltaTime) 
{
    double speed = moveSpeed * deltaTime;
    if (_shiftPressed) {
        speed *= 2.0;
    }

    vec3 movement(0);

    if (_keyW) movement += _transform.fwd();
    if (_keyS) movement -= _transform.fwd();
    if (_keyA) movement += _transform.left(); 
    if (_keyD) movement -= _transform.left(); 

    if (glm::length(movement) > 0.001) {
        movement = glm::normalize(movement) * speed;
        _transform.translate(movement);
    }
}

void Camera::handlePan(int deltaX, int deltaY)
{
    // 鼠标右移 -> 往右平移；鼠标上移 -> 往上平移 Mueve el raton hacia la derecha -> muevelo hacia la derecha; Mueve el raton hacia arriba -> muevelo hacia arriba.
    // 注意：right = -left()
    vec3 right = -_transform.left();
    vec3 up = _transform.up();

    vec3 delta = (-deltaX * panSpeed) * right + (deltaY * panSpeed) * up;
    _transform.translate(delta);

    // 如果你希望“轨道目标”也跟着一起平移（更符合轨道相机手感），就把下一行解注释：El "objetivo orbital" tambien se desplaza con el (para adaptarse mejor a la sensacion de una camara orbital), por lo que la siguiente linea esta anotada
    // orbitTarget += delta;
}


void Camera::handleOrbit(int deltaX, int deltaY) 
{
    _yaw -= deltaX * lookSensitivity;
    _pitch -= deltaY * lookSensitivity;

    const double maxPitch = glm::radians(89.0);
    _pitch = glm::clamp(_pitch, -maxPitch, maxPitch);

 double cosYaw = cos(_yaw);
    double sinYaw = sin(_yaw);
    double cosPitch = cos(_pitch);
    double sinPitch = sin(_pitch);

    vec3 offset;
    offset.x = orbitDistance * cosPitch * sinYaw;
    offset.y = orbitDistance * sinPitch;
    offset.z = orbitDistance * cosPitch * cosYaw;

    _transform.pos() = orbitTarget + offset;

    updateOrientation();
}