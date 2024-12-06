#pragma once

#include <cmath>
#include "structure/math/spk_vector3.hpp"

namespace spk
{
	class Quaternion
	{
	public:
		float x, y, z, w;

		Quaternion();
		Quaternion(float p_x, float p_y, float p_z, float p_w);

		static Quaternion identity();

		static Quaternion fromEuler(const Vector3 &p_euler);
		Vector3 toEuler() const;

		static Quaternion fromAxisAngle(const Vector3 &p_axis, float p_angle);

		void normalize();
		Quaternion normalized() const;

		Quaternion operator*(const Quaternion &p_rhs) const;
		Vector3 operator*(const Vector3 &p_rhs) const;

		Vector3 rotate(const Vector3 &p_v) const;
	};
}
