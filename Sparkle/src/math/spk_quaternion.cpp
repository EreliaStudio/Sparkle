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

	Quaternion Quaternion::operator+(const Quaternion& rhs) const
	{
		return Quaternion(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
	}

	Quaternion& Quaternion::operator+=(const Quaternion& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		w += rhs.w;
		return *this;
	}

	Quaternion Quaternion::operator-(const Quaternion& rhs) const
	{
		return Quaternion(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
	}

	Quaternion& Quaternion::operator-=(const Quaternion& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		w -= rhs.w;
		return *this;
	}

	Quaternion Quaternion::operator*(const float& scalar) const
	{
		return Quaternion(x * scalar, y * scalar, z * scalar, w * scalar);
	}

	Quaternion& Quaternion::operator*=(const float& scalar)
	{
		x *= scalar;
		y *= scalar;
		z *= scalar;
		w *= scalar;
		return *this;
	}

	Quaternion Quaternion::operator/(const float& scalar) const
	{
		return Quaternion(x / scalar, y / scalar, z / scalar, w / scalar);
	}

	Quaternion& Quaternion::operator/=(const float& scalar)
	{
		x /= scalar;
		y /= scalar;
		z /= scalar;
		w /= scalar;
		return *this;
	}

	bool Quaternion::operator==(const Quaternion& p_rhs) const
	{
		return x == p_rhs.x && y == p_rhs.y && z == p_rhs.z && w == p_rhs.w;
	}

	bool Quaternion::operator!=(const Quaternion& p_rhs) const
	{
		return !(*this == p_rhs);
	}

	bool Quaternion::operator<(const Quaternion& p_rhs) const
	{
		if (x != p_rhs.x) return x < p_rhs.x;
        if (y != p_rhs.y) return y < p_rhs.y;
        if (z != p_rhs.z) return z < p_rhs.z;
        return w < p_rhs.w;
	}

	bool Quaternion::operator>(const Quaternion& p_rhs) const
	{
		return !(*this < p_rhs || *this == p_rhs);
	}

	bool Quaternion::operator<=(const Quaternion& p_rhs) const
	{
		return !(*this > p_rhs);
	}

	bool Quaternion::operator>=(const Quaternion& p_rhs) const
	{
		return !(*this < p_rhs);
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
        Vector3 right = p_up.cross(forward).normalize();
        Vector3 up = forward.cross(right);

        // Create a rotation matrix from the right, up, and forward vectors
        float matrix[3][3] = {
            {right.x, up.x, forward.x},
            {right.y, up.y, forward.y},
            {right.z, up.z, forward.z}
        };

        // Convert the rotation matrix to a quaternion
        float trace = matrix[0][0] + matrix[1][1] + matrix[2][2];
        float qw, qx, qy, qz;

        if (trace > 0.0f) {
            float s = 0.5f / sqrt(trace + 1.0f);
            qw = 0.25f / s;
            qx = (matrix[2][1] - matrix[1][2]) * s;
            qy = (matrix[0][2] - matrix[2][0]) * s;
            qz = (matrix[1][0] - matrix[0][1]) * s;
        } else {
            if (matrix[0][0] > matrix[1][1] && matrix[0][0] > matrix[2][2]) {
                float s = 2.0f * sqrt(1.0f + matrix[0][0] - matrix[1][1] - matrix[2][2]);
                qw = (matrix[2][1] - matrix[1][2]) / s;
                qx = 0.25f * s;
                qy = (matrix[0][1] + matrix[1][0]) / s;
                qz = (matrix[0][2] + matrix[2][0]) / s;
            } else if (matrix[1][1] > matrix[2][2]) {
                float s = 2.0f * sqrt(1.0f + matrix[1][1] - matrix[0][0] - matrix[2][2]);
                qw = (matrix[0][2] - matrix[2][0]) / s;
                qx = (matrix[0][1] + matrix[1][0]) / s;
                qy = 0.25f * s;
                qz = (matrix[1][2] + matrix[2][1]) / s;
            } else {
                float s = 2.0f * sqrt(1.0f + matrix[2][2] - matrix[0][0] - matrix[1][1]);
                qw = (matrix[1][0] - matrix[0][1]) / s;
                qx = (matrix[0][2] + matrix[2][0]) / s;
                qy = (matrix[1][2] + matrix[2][1]) / s;
                qz = 0.25f * s;
            }
        }

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

	Quaternion Quaternion::fromEulerAngles(const float& p_angleX, const float& p_angleY, const float& p_angleZ)
	{
		return (fromEulerAngles(spk::Vector3(p_angleX, p_angleY, p_angleZ)));
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

	Vector3 Quaternion::toVector3() const
	{
		return (spk::Vector3(x, y, z));
	}
}
