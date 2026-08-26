#pragma once

#include <numbers>

#include "math/vector3.hpp"

namespace spk
{
	class Quaternion
	{
	private:
		static constexpr float radians(float degrees) noexcept
		{
			return degrees * std::numbers::pi_v<float> / 180.0f;
		}

		static constexpr float degrees(float radians) noexcept
		{
			return radians * 180.0f / std::numbers::pi_v<float>;
		}

	public:
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		float w = 1.0f;

		constexpr Quaternion() noexcept = default;

		constexpr Quaternion(float x, float y, float z, float w) noexcept :
			x(x),
			y(y),
			z(z),
			w(w)
		{
		}

		[[nodiscard]] constexpr bool operator==(const Quaternion&) const noexcept = default;

		[[nodiscard]] static constexpr Quaternion identity() noexcept
		{
			return {};
		}

		[[nodiscard]] constexpr Quaternion operator*(const Quaternion& other) const noexcept
		{
			return {
				w * other.x + x * other.w + y * other.z - z * other.y,
				w * other.y - x * other.z + y * other.w + z * other.x,
				w * other.z + x * other.y - y * other.x + z * other.w,
				w * other.w - x * other.x - y * other.y - z * other.z
			};
		}

		[[nodiscard]] constexpr Quaternion conjugated() const noexcept
		{
			return {-x, -y, -z, w};
		}

		[[nodiscard]] Quaternion inversed() const
		{
			const float squaredNorm = dot(*this);

			if (squaredNorm == 0.0f)
			{
				throw std::runtime_error("Can't inverse a null quaternion");
			}

			const Quaternion conjugate = conjugated();

			return {
				conjugate.x / squaredNorm,
				conjugate.y / squaredNorm,
				conjugate.z / squaredNorm,
				conjugate.w / squaredNorm
			};
		}

		[[nodiscard]] static Quaternion fromAxisAngle(const Vector3 &axis, float angle);
		[[nodiscard]] static Quaternion fromEuler(const Vector3 &angles);

		[[nodiscard]] Quaternion normalized() const;
		[[nodiscard]] constexpr float dot(const Quaternion &q) const noexcept
		{
			return x * q.x + y * q.y + z * q.z + w * q.w;
		}

		[[nodiscard]] Vector3 toEuler() const;

		[[nodiscard]] static Quaternion slerp(const Quaternion &from, const Quaternion &to, float alpha);
		[[nodiscard]] static Quaternion lookAt(const Vector3 &from, const Vector3 &to, const Vector3 &up = {0, 1, 0});
	};
}