#include "math/spk_quaternion.hpp"
#include "spk_basic_functions.hpp"

namespace spk
{
	Quaternion::Quaternion() :
		x(0), y(0), z(0), w(1) 
	{
	}

	Quaternion::Quaternion(const float& p_x, const float& p_y, const float& p_z, const float& p_w) :
		x(p_x), y(p_y), z(p_z), w(p_w) 
	{
	}

	Quaternion::Quaternion(const spk::Vector3& p_eulerAngles)
	{
		*this = Quaternion();
		*this *= spk::Quaternion::fromAxisAngle(spk::Vector3(1, 0, 0), p_eulerAngles.x);
		*this *= spk::Quaternion::fromAxisAngle(spk::Vector3(0, 1, 0), p_eulerAngles.y);
		*this *= spk::Quaternion::fromAxisAngle(spk::Vector3(0, 0, 1), p_eulerAngles.z);
	}

	Quaternion Quaternion::normalize() const
	{
		float length = std::sqrt(x * x + y * y + z * z + w * w);
		return Quaternion(x / length, y / length, z / length, w / length);
	}

	Quaternion Quaternion::inverse() const
	{
		return conjugate().normalize();
	}

	Quaternion Quaternion::conjugate() const
	{
		return Quaternion(-x, -y, -z, w);
	}

	Quaternion Quaternion::operator*(const Quaternion& p_rhs) const
	{
		return Quaternion(
			w * p_rhs.x + x * p_rhs.w + y * p_rhs.z - z * p_rhs.y,
			w * p_rhs.y + y * p_rhs.w + z * p_rhs.x - x * p_rhs.z,
			w * p_rhs.z + z * p_rhs.w + x * p_rhs.y - y * p_rhs.x,
			w * p_rhs.w - x * p_rhs.x - y * p_rhs.y - z * p_rhs.z
		);
	}

	Quaternion& Quaternion::operator*=(const Quaternion& p_rhs)
	{
		*this = *this * p_rhs;
		return *this;
	}

	Quaternion Quaternion::lookAt(const Vector3& p_direction, const Vector3& p_up)
	{
		Vector3 forward = p_direction.normalize();

		Vector3 upVector = p_up;
		if (fabs(forward.dot(upVector)) > 0.9999f)
		{
			upVector = fabs(forward.x) < fabs(forward.z) ? Vector3(1, 0, 0) : Vector3(0, 1, 0);
		}

		Vector3 right = upVector.cross(forward).normalize();
		Vector3 up = forward.cross(right);

		float matrix[3][3] = {
				{right.x, right.y, right.z},
				{up.x, up.y, up.z},
				{-forward.x, -forward.y, -forward.z}
			};
		float qw = sqrt(1.0f + matrix[0][0] + matrix[1][1] + matrix[2][2]) / 2.0f;

		if (qw <= 0.0f)
		{
			const float half = 0.5f;
			float qx, qy, qz;

			if (matrix[0][0] > matrix[1][1] && matrix[0][0] > matrix[2][2]) {
				qx = sqrt(1.0f + matrix[0][0] - matrix[1][1] - matrix[2][2]) * half;
				qy = (matrix[0][1] + matrix[1][0]) / (4.0f * qx);
				qz = (matrix[0][2] + matrix[2][0]) / (4.0f * qx);
				qw = 0.0f;
			} else if (matrix[1][1] > matrix[2][2]) {
				qy = sqrt(1.0f - matrix[0][0] + matrix[1][1] - matrix[2][2]) * half;
				qx = (matrix[0][1] + matrix[1][0]) / (4.0f * qy);
				qz = (matrix[1][2] + matrix[2][1]) / (4.0f * qy);
				qw = 0.0f;
			} else {
				qz = sqrt(1.0f - matrix[0][0] - matrix[1][1] + matrix[2][2]) * half;
				qx = (matrix[0][2] + matrix[2][0]) / (4.0f * qz);
				qy = (matrix[1][2] + matrix[2][1]) / (4.0f * qz);
				qw = 0.0f;
			}

			return Quaternion(qx, qy, qz, qw);
		}
		float w4 = 4.0f * qw;
		float qx = (matrix[2][1] - matrix[1][2]) / w4;
		float qy = (matrix[0][2] - matrix[2][0]) / w4;
		float qz = (matrix[1][0] - matrix[0][1]) / w4;

		return Quaternion(qx, qy, qz, qw).normalize();
	}


	Quaternion Quaternion::fromEulerAngles(const Vector3& p_eulerAngles)
	{
		float radianX = spk::degreeToRadian(p_eulerAngles.x);
		float radianY = spk::degreeToRadian(p_eulerAngles.y);
		float radianZ = spk::degreeToRadian(p_eulerAngles.z);

		float cy = cos(radianZ * 0.5f);
		float sy = sin(radianZ * 0.5f);
		float cp = cos(radianY * 0.5f);
		float sp = sin(radianY * 0.5f);
		float cr = cos(radianX * 0.5f);
		float sr = sin(radianX * 0.5f);

		return Quaternion(
			sr * cp * cy - cr * sp * sy,
			cr * sp * cy + sr * cp * sy,
			cr * cp * sy - sr * sp * cy,
			cr * cp * cy + sr * sp * sy
		);
	}

	Quaternion Quaternion::fromAxisAngle(const Vector3& p_axis, float p_angleDegree)
	{
		float halfAngle = spk::degreeToRadian(p_angleDegree) * 0.5f;
		float s = sin(halfAngle);
		return Quaternion(
			p_axis.x * s,
			p_axis.y * s,
			p_axis.z * s,
			cos(halfAngle)
		);
	}

	Vector3 Quaternion::applyRotation(const spk::Vector3& p_inputPoint) const
	{
		Quaternion point(p_inputPoint.x, p_inputPoint.y, p_inputPoint.z, 0);
		Quaternion result = *this * point * this->normalize().inverse();
		return Vector3(result.x, result.y, result.z);
	}
}
