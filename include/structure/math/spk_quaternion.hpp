#pragma once

#include "structure/math/spk_vector3.hpp"
#include <cmath>

namespace spk
{
	class Quaternion
	{
	public:
		float x, y, z, w;

		Quaternion();
		Quaternion(float p_x, float p_y, float p_z, float p_w);

		friend std::wostream &operator<<(std::wostream &p_os, const Quaternion &p_vec)
		{
			p_os << L"(" << p_vec.x << L", " << p_vec.y << L", " << p_vec.z << L", " << p_vec.w << L")";
			return p_os;
		}

		friend std::ostream &operator<<(std::ostream &p_os, const Quaternion &p_vec)
		{
			p_os << "(" << p_vec.x << ", " << p_vec.y << ", " << p_vec.z << ", " << p_vec.w << ")";
			return p_os;
		}

		std::wstring to_wstring() const;
		std::string to_string() const;

		static Quaternion identity();

		static Quaternion fromEuler(const Vector3 &p_euler);
		Vector3 toEuler() const;

		static Quaternion fromAxisAngle(const Vector3 &p_axis, float p_angle);

		Quaternion normalize() const;

		Quaternion operator*(const Quaternion &p_rhs) const;
		Vector3 operator*(const Vector3 &p_rhs) const;

		Vector3 rotate(const Vector3 &p_v) const;

		static Quaternion lookAt(const Vector3 &eye, const Vector3 &target, const Vector3 &p_up);
	};
}
