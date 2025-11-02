#pragma once

#include "types.h"

class Transform {

	union {
		mat4 _mat = mat4(1.0);
		struct {
			vec3 _left; mat4::value_type _left_w;
			vec3 _up; mat4::value_type _up_w;
			vec3 _fwd; mat4::value_type _fwd_w;
			vec3 _pos; mat4::value_type _pos_w;
		};
	};

public:
	const auto& mat() const { return _mat; }
	auto& mat_mutable() { return _mat; }
	const auto& left() const { return _left; }
	const auto& up() const { return _up; }
	const auto& fwd() const { return _fwd; }
	const auto& pos() const { return _pos; }
	auto& pos() { return _pos; }

	void translate(const vec3& v);
	void rotate(double rads, const vec3& v);

	void setPosition(const vec3& t);
	vec3 getScale() const;
	void setScale(const vec3& s);
	void resetScale();
	void resetRotation();

	void rotateEulerDeltaDeg(const vec3& deltaDegXYZ);

};