#include "structure/math/spk_quaternion.hpp"

#include "structure/spk_iostream.hpp"

namespace spk
{
	Quaternion::Quaternion() :
		x(0.f),
		y(0.f),
		z(0.f),
		w(1.f)
	{
	}
	Quaternion::Quaternion(float p_x, float p_y, float p_z, float p_w) :
		x(p_x),
		y(p_y),
		z(p_z),
		w(p_w)
	{
	}

	std::wstring Quaternion::to_wstring() const
	{
		std::wstringstream wss;
		wss << *this;
		return wss.str();
	}

	std::string Quaternion::to_string() const
	{
		std::stringstream ss;
		ss << *this;
		return ss.str();
	}

	Quaternion Quaternion::identity()
	{
		return Quaternion(0.f, 0.f, 0.f, 1.f);
	}

	Quaternion Quaternion::fromEuler(const Vector3 &p_euler)
	{
		float pitch = spk::degreeToRadian(p_euler.y);
		float yaw = spk::degreeToRadian(p_euler.z);
		float roll = spk::degreeToRadian(p_euler.x);

		double cr = cos(roll * 0.5);
		double sr = sin(roll * 0.5);
		double cp = cos(pitch * 0.5);
		double sp = sin(pitch * 0.5);
		double cy = cos(yaw * 0.5);
		double sy = sin(yaw * 0.5);

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
		{
			euler.x = std::copysign((float)M_PI / 2.f, sinp);
		}
		else
		{
			euler.x = std::asin(sinp);
		}

		float siny_cosp = 2.f * (w * z + x * y);
		float cosy_cosp = 1.f - 2.f * (y * y + z * z);
		euler.y = std::atan2(siny_cosp, cosy_cosp);

		euler.x = spk::radianToDegree(euler.x);
		euler.y = spk::radianToDegree(euler.y);
		euler.z = spk::radianToDegree(euler.z);

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

	Quaternion Quaternion::normalize() const
	{
		Quaternion q;
		float mag = std::sqrt(x * x + y * y + z * z + w * w);
		if (mag > 0.f)
		{
			float invMag = 1.f / mag;

			q.x = x * invMag;
			q.y = y * invMag;
			q.z = z * invMag;
			q.w = w * invMag;
		}
		else
		{
			q.x = 0.f;
			q.y = 0.f;
			q.z = 0.f;
			q.w = 1.f;
		}
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

	Quaternion Quaternion::lookAt(const Vector3 &p_eye, const Vector3 &p_target, const Vector3 &p_up)
	{
		Vector3 forward = (p_target - p_eye).normalize();

		float forwardToUpDot = forward.dot(p_up);
		Vector3 right = Vector3(1, 0, 0);
		if (forwardToUpDot != -1 && forwardToUpDot != 1)
		{
			right = forward.cross(p_up).normalize();
		}

		Vector3 up = right.cross(forward);

		float m[3][3];
		m[0][0] = right.x;
		m[0][1] = up.x;
		m[0][2] = forward.x;
		m[1][0] = right.y;
		m[1][1] = up.y;
		m[1][2] = forward.y;
		m[2][0] = right.z;
		m[2][1] = up.z;
		m[2][2] = forward.z;

		Quaternion q;
		float trace = m[0][0] + m[1][1] + m[2][2];

		if (trace > 0.0f)
		{
			float s = std::sqrt(trace + 1.0f) * 2.0f;
			q.w = s * 0.25f;
			q.x = (m[2][1] - m[1][2]) / s;
			q.y = (m[0][2] - m[2][0]) / s;
			q.z = (m[1][0] - m[0][1]) / s;
		}
		else
		{
			if (m[0][0] > m[1][1] && m[0][0] > m[2][2])
			{
				float s = std::sqrt(1.0f + m[0][0] - m[1][1] - m[2][2]) * 2.0f;
				q.x = 0.25f * s;
				q.y = (m[0][1] + m[1][0]) / s;
				q.z = (m[0][2] + m[2][0]) / s;
				q.w = (m[2][1] - m[1][2]) / s;
			}
			else if (m[1][1] > m[2][2])
			{
				float s = std::sqrt(1.0f + m[1][1] - m[0][0] - m[2][2]) * 2.0f;
				q.x = (m[0][1] + m[1][0]) / s;
				q.y = 0.25f * s;
				q.z = (m[1][2] + m[2][1]) / s;
				q.w = (m[0][2] - m[2][0]) / s;
			}
			else
			{
				float s = std::sqrt(1.0f + m[2][2] - m[0][0] - m[1][1]) * 2.0f;
				q.x = (m[0][2] + m[2][0]) / s;
				q.y = (m[1][2] + m[2][1]) / s;
				q.z = 0.25f * s;
				q.w = (m[1][0] - m[0][1]) / s;
			}
		}

		return q.normalize();
	}
}