#pragma once
#include "Transform.h"

class Camera {

public:
	double fov = glm::radians(60.0);
	double aspect = 16.0 / 9.0;
	double zNear = 0.1;
	double zFar = 128.0;
	double moveSpeed = 2.0;
	double lookSensitivity = 0.003;
	double zoomSpeed = 0.5;
	double panSpeed = 0.01;
	double orbitDistance = 5.0;
	vec3 orbitTarget = vec3(0, 0, 0);

private:
	Transform _transform;

	bool _rightMouseDown = false;
	bool _leftMouseDown = false;
	bool _middleMouseDown = false;
	bool _altPressed = false;
	bool _shiftPressed = false;
	int _lastMouseX = 0;
	int _lastMouseY = 0;
	bool _keyW = false;
	bool _keyA = false;
	bool _keyS = false;
	bool _keyD = false;
	double _yaw = 0.0;
	double _pitch = 0.0;

public:
	const auto& transform() const { return _transform; }
	auto& transform() { return _transform; }
	mat4 projection() const;
	mat4 view() const;
	void onMouseButton(int button, int state, int x, int y);
	void onMouseMove(int x, int y);
	void onMouseWheel(int direction);
	void onKeyDown(unsigned char key);
	void onKeyUp(unsigned char key);
	void onSpecialKeyDown(int key);
	void onSpecialKeyUp(int key);
	void update(double deltaTime);
	void focusOn(const vec3& target, double distance = 5.0);

private:
	void updateOrientation();
	void handleFPSMovement(double deltaTime);
	void handleOrbit(int deltaX, int deltaY);
	void handlePan(int deltaX, int deltaY);
};