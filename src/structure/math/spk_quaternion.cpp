#include "structure/math/spk_quaternion.hpp"

namespace spk
{
	Quaternion::Quaternion() : x(0.f), y(0.f), z(0.f), w(1.f) {}
	Quaternion::Quaternion(float p_x, float p_y, float p_z, float p_w) : x(p_x), y(p_y), z(p_z), w(p_w) {}

	Quaternion Quaternion::identity() { return Quaternion(0.f, 0.f, 0.f, 1.f); }

	Quaternion Quaternion::fromEuler(const Vector3 &p_euler)
	{
		float pitch = p_euler.x;
		float yaw = p_euler.y;
		float roll = p_euler.z;

		float cy = std::cos(yaw * 0.5f);
		float sy = std::sin(yaw * 0.5f);
		float cp = std::cos(pitch * 0.5f);
		float sp = std::sin(pitch * 0.5f);
		float cr = std::cos(roll * 0.5f);
		float sr = std::sin(roll * 0.5f);

		Quaternion q;
		q.w = cr * cp * cy + sr * sp * sy;
		q.x = sr * cp * cy - cr * sp * sy;
		q.y = cr * sp * cy + sr * cp * sy;
		q.z = cr * cp * sy - sr * sp * cy;
		return q;
	}

	Vector3 Quaternion::toEuler() const
	{
		Vector3 euler;

		float sinr_cosp = 2.f * (w * x + y * z);
		float cosr_cosp = 1.f - 2.f * (x * x + y * y);
		euler.z = std::atan2(sinr_cosp, cosr_cosp);

		float sinp = 2.f * (w * y - z * x);
		if (std::abs(sinp) >= 1.f)
			euler.x = std::copysign((float)M_PI / 2.f, sinp);
		else
			euler.x = std::asin(sinp);

		float siny_cosp = 2.f * (w * z + x * y);
		float cosy_cosp = 1.f - 2.f * (y * y + z * z);
		euler.y = std::atan2(siny_cosp, cosy_cosp);

		return euler;
	}

	Quaternion Quaternion::fromAxisAngle(const Vector3 &p_axis, float p_angle)
	{
		Vector3 normAxis = p_axis.normalize();
		float halfAngle = spk::degreeToRadian(p_angle) * 0.5f;
		float s = std::sin(halfAngle);

		Quaternion q;
		q.w = std::cos(halfAngle);
		q.x = normAxis.x * s;
		q.y = normAxis.y * s;
		q.z = normAxis.z * s;
		return q;
	}

	void Quaternion::normalize()
	{
		float mag = std::sqrt(x * x + y * y + z * z + w * w);
		if (mag > 0.f)
		{
			float invMag = 1.f / mag;
			x *= invMag;
			y *= invMag;
			z *= invMag;
			w *= invMag;
		}
		else
		{
			x = y = z = 0.f;
			w = 1.f;
		}
	}

	Quaternion Quaternion::normalized() const
	{
		Quaternion q(*this);
		q.normalize();
		return q;
	}

	Quaternion Quaternion::operator*(const Quaternion &p_rhs) const
	{
		Quaternion q;
		q.w = w * p_rhs.w - x * p_rhs.x - y * p_rhs.y - z * p_rhs.z;
		q.x = w * p_rhs.x + x * p_rhs.w + y * p_rhs.z - z * p_rhs.y;
		q.y = w * p_rhs.y - x * p_rhs.z + y * p_rhs.w + z * p_rhs.x;
		q.z = w * p_rhs.z + x * p_rhs.y - y * p_rhs.x + z * p_rhs.w;
		return q;
	}

	Vector3 Quaternion::operator*(const Vector3 &p_rhs) const
	{
		Quaternion qv(p_rhs.x, p_rhs.y, p_rhs.z, 0.f);
		Quaternion qInv(-x, -y, -z, w);
		Quaternion result = (*this) * qv * qInv;
		return Vector3(result.x, result.y, result.z);
	}

	Vector3 Quaternion::rotate(const Vector3 &p_vector) const
	{
		return (this->operator*(p_vector));
	}
}