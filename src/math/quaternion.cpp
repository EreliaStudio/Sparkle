#include "math/quaternion.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace spk
{
	Quaternion Quaternion::fromAxisAngle(const Vector3 &axis, float angle)
	{
		const Vector3 n = axis.normalized();
		const float half = radians(angle) * 0.5f;
		const float s = std::sin(half);

		return {
			n.x * s,
			n.y * s,
			n.z * s,
			std::cos(half)};
	}

	Quaternion Quaternion::fromEuler(const Vector3 &angles)
	{
		const float hx = radians(angles.x) * 0.5f;
		const float hy = radians(angles.y) * 0.5f;
		const float hz = radians(angles.z) * 0.5f;

		const float cx = std::cos(hx);
		const float sx = std::sin(hx);
		const float cy = std::cos(hy);
		const float sy = std::sin(hy);
		const float cz = std::cos(hz);
		const float sz = std::sin(hz);

		return {
			sx * cy * cz - cx * sy * sz,
			cx * sy * cz + sx * cy * sz,
			cx * cy * sz - sx * sy * cz,
			cx * cy * cz + sx * sy * sz};
	}

	Quaternion Quaternion::normalized() const
	{
		const float length = std::sqrt(x * x + y * y + z * z + w * w);
		if (length == 0.0f)
		{
			throw std::domain_error("Cannot normalize a zero-length quaternion");
		}

		return {
			x / length,
			y / length,
			z / length,
			w / length};
	}

	Vector3 Quaternion::toEuler() const
	{
		const Quaternion q = normalized();

		const float sinr = 2.0f * (q.w * q.x + q.y * q.z);
		const float cosr = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);

		const float sinp = std::clamp(2.0f * (q.w * q.y - q.z * q.x), -1.0f, 1.0f);

		const float siny = 2.0f * (q.w * q.z + q.x * q.y);
		const float cosy = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);

		return {
			degrees(std::atan2(sinr, cosr)),
			degrees(std::asin(sinp)),
			degrees(std::atan2(siny, cosy))};
	}

	Quaternion Quaternion::slerp(const Quaternion &from, const Quaternion &to, float alpha)
	{
		Quaternion a = from.normalized();
		Quaternion b = to.normalized();

		float d = a.dot(b);
		if (d < 0.0f)
		{
			b = {-b.x, -b.y, -b.z, -b.w};
			d = -d;
		}

		if (d > 0.9995f)
		{
			return Quaternion{
				a.x + alpha * (b.x - a.x),
				a.y + alpha * (b.y - a.y),
				a.z + alpha * (b.z - a.z),
				a.w + alpha * (b.w - a.w)}
				.normalized();
		}

		const float theta = std::acos(std::clamp(d, -1.0f, 1.0f));
		const float s = std::sin(theta);
		const float u = std::sin((1.0f - alpha) * theta) / s;
		const float v = std::sin(alpha * theta) / s;

		return {
			u * a.x + v * b.x,
			u * a.y + v * b.y,
			u * a.z + v * b.z,
			u * a.w + v * b.w};
	}

	Quaternion Quaternion::lookAt(const Vector3 &from, const Vector3 &to, const Vector3 &up)
	{
		const Vector3 forward = (to - from).normalized();

		Vector3 right = forward.cross(up);
		if (right.length() < 0.000001f)
		{
			right = forward.cross(
				std::abs(forward.y) < 0.999f
					? Vector3{0, 1, 0}
					: Vector3{1, 0, 0});
		}

		right = right.normalized();

		const Vector3 correctedUp = right.cross(forward);

		const float m00 = right.x;
		const float m01 = correctedUp.x;
		const float m02 = -forward.x;

		const float m10 = right.y;
		const float m11 = correctedUp.y;
		const float m12 = -forward.y;

		const float m20 = right.z;
		const float m21 = correctedUp.z;
		const float m22 = -forward.z;

		Quaternion q;

		const float trace = m00 + m11 + m22;
		if (trace > 0.0f)
		{
			const float s = std::sqrt(trace + 1.0f) * 2.0f;
			q.w = 0.25f * s;
			q.x = (m21 - m12) / s;
			q.y = (m02 - m20) / s;
			q.z = (m10 - m01) / s;
		}
		else if (m00 > m11 && m00 > m22)
		{
			const float s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;
			q.w = (m21 - m12) / s;
			q.x = 0.25f * s;
			q.y = (m01 + m10) / s;
			q.z = (m02 + m20) / s;
		}
		else if (m11 > m22)
		{
			const float s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;
			q.w = (m02 - m20) / s;
			q.x = (m01 + m10) / s;
			q.y = 0.25f * s;
			q.z = (m12 + m21) / s;
		}
		else
		{
			const float s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;
			q.w = (m10 - m01) / s;
			q.x = (m02 + m20) / s;
			q.y = (m12 + m21) / s;
			q.z = 0.25f * s;
		}

		return q.normalized();
	}
}