#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <GL/freeglut.h>
#include <algorithm>

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
	if (button == GLUT_RIGHT_BUTTON) {
		_rightMouseDown = (state == GLUT_DOWN);
		if (_rightMouseDown) {
			_lastMouseX = x;
			_lastMouseY = y;
			glutSetCursor(GLUT_CURSOR_NONE);
		}
		else {
			glutSetCursor(GLUT_CURSOR_INHERIT);
		}
	}
	else if (button == GLUT_LEFT_BUTTON) {
		_leftMouseDown = (state == GLUT_DOWN);
		if (_leftMouseDown) {
			_lastMouseX = x;
			_lastMouseY = y;
		}
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

		int windowWidth = glutGet(GLUT_WINDOW_WIDTH);
		int windowHeight = glutGet(GLUT_WINDOW_HEIGHT);
		int centerX = windowWidth / 2;
		int centerY = windowHeight / 2;

		glutWarpPointer(centerX, centerY);
		_lastMouseX = centerX;
		_lastMouseY = centerY;
	}
	else if (_leftMouseDown && _altPressed) 
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
	case 'w': case 'W': _keyW = true; break;
	case 'a': case 'A': _keyA = true; break;
	case 's': case 'S': _keyS = true; break;
	case 'd': case 'D': _keyD = true; break;
	case 'f': case 'F': focusOn(orbitTarget, orbitDistance); break;
	}
}

void Camera::onKeyUp(unsigned char key) 
{
	switch (key) {
	case 'w': case 'W': _keyW = false; break;
	case 'a': case 'A': _keyA = false; break;
	case 's': case 'S': _keyS = false; break;
	case 'd': case 'D': _keyD = false; break;
	}
}

void Camera::onSpecialKeyDown(int key) 
{
	switch (key) {
	case GLUT_KEY_SHIFT_L:
	case GLUT_KEY_SHIFT_R:
		_shiftPressed = true;
		break;
	case GLUT_KEY_ALT_L:
	case GLUT_KEY_ALT_R:
		_altPressed = true;
		break;
	}
}

void Camera::onSpecialKeyUp(int key) 
{
	switch (key) {
	case GLUT_KEY_SHIFT_L:
	case GLUT_KEY_SHIFT_R:
		_shiftPressed = false;
		break;
	case GLUT_KEY_ALT_L:
	case GLUT_KEY_ALT_R:
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