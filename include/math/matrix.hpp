#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <type_traits>

#include "math/quaternion.hpp"
#include "math/vector4.hpp"

namespace spk
{
	template <std::size_t SizeX, std::size_t SizeY>
	class Matrix
	{
	public:
		class Column
		{
		private:
			std::array<float, SizeY> _rows{};

		public:
			[[nodiscard]] constexpr float &operator[](std::size_t row)
			{
				return _rows.at(row);
			}
			[[nodiscard]] constexpr const float &operator[](std::size_t row) const
			{
				return _rows.at(row);
			}
		};

	private:
		std::array<Column, SizeX> _columns{};

	public:
		[[nodiscard]] constexpr Column &operator[](std::size_t column)
		{
			return _columns.at(column);
		}
		[[nodiscard]] constexpr const Column &operator[](std::size_t column) const
		{
			return _columns.at(column);
		}
		[[nodiscard]] static constexpr Matrix identity()
			requires(SizeX == SizeY)
		{
			Matrix result;
			for (std::size_t i = 0; i < SizeX; ++i)
			{
				result[i][i] = 1.0f;
			}
			return result;
		}
		[[nodiscard]] constexpr Matrix operator*(const Matrix &other) const
			requires(SizeX == SizeY)
		{
			Matrix result;
			for (std::size_t c = 0; c < SizeX; ++c)
			{
				for (std::size_t r = 0; r < SizeY; ++r)
				{
					for (std::size_t k = 0; k < SizeX; ++k)
					{
						result[c][r] += (*this)[k][r] * other[c][k];
					}
				}
			}
			return result;
		}
		[[nodiscard]] constexpr Vector4 operator*(const Vector4 &v) const
			requires(SizeX == 4 && SizeY == 4)
		{
			return {(*this)[0][0] * v.x + (*this)[1][0] * v.y + (*this)[2][0] * v.z + (*this)[3][0] * v.w, (*this)[0][1] * v.x + (*this)[1][1] * v.y + (*this)[2][1] * v.z + (*this)[3][1] * v.w, (*this)[0][2] * v.x + (*this)[1][2] * v.y + (*this)[2][2] * v.z + (*this)[3][2] * v.w, (*this)[0][3] * v.x + (*this)[1][3] * v.y + (*this)[2][3] * v.z + (*this)[3][3] * v.w};
		}
		[[nodiscard]] static Matrix translation(float x, float y, float z)
			requires(SizeX == 4 && SizeY == 4)
		{
			auto m = identity();
			m[3][0] = x;
			m[3][1] = y;
			m[3][2] = z;
			return m;
		}
		[[nodiscard]] static Matrix translation(const Vector3 &v)
			requires(SizeX == 4 && SizeY == 4)
		{
			return translation(v.x, v.y, v.z);
		}
		[[nodiscard]] static Matrix scale(float x, float y, float z)
			requires(SizeX == 4 && SizeY == 4)
		{
			auto m = identity();
			m[0][0] = x;
			m[1][1] = y;
			m[2][2] = z;
			return m;
		}
		[[nodiscard]] static Matrix scale(const Vector3 &v)
			requires(SizeX == 4 && SizeY == 4)
		{
			return scale(v.x, v.y, v.z);
		}
		[[nodiscard]] static Matrix rotation(const Quaternion &value)
			requires(SizeX == 4 && SizeY == 4)
		{
			const auto q = value.normalized();
			const float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z, xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z, wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
			auto m = identity();
			m[0][0] = 1 - 2 * (yy + zz);
			m[0][1] = 2 * (xy + wz);
			m[0][2] = 2 * (xz - wy);
			m[1][0] = 2 * (xy - wz);
			m[1][1] = 1 - 2 * (xx + zz);
			m[1][2] = 2 * (yz + wx);
			m[2][0] = 2 * (xz + wy);
			m[2][1] = 2 * (yz - wx);
			m[2][2] = 1 - 2 * (xx + yy);
			return m;
		}
		[[nodiscard]] static Matrix rotation(float x, float y, float z)
			requires(SizeX == 4 && SizeY == 4)
		{
			return rotation(Quaternion::fromEuler({x, y, z}));
		}
		[[nodiscard]] static Matrix rotation(const Vector3 &v)
			requires(SizeX == 4 && SizeY == 4)
		{
			return rotation(v.x, v.y, v.z);
		}
		[[nodiscard]] static Matrix ortho(float left, float right, float bottom, float top, float nearPlane = -1.0f, float farPlane = 1.0f)
			requires(SizeX == 4 && SizeY == 4)
		{
			if (right == left || top == bottom || farPlane == nearPlane)
			{
				throw std::invalid_argument("Invalid orthographic projection bounds");
			}
			auto m = identity();
			m[0][0] = 2 / (right - left);
			m[1][1] = 2 / (top - bottom);
			m[2][2] = -2 / (farPlane - nearPlane);
			m[3][0] = -(right + left) / (right - left);
			m[3][1] = -(top + bottom) / (top - bottom);
			m[3][2] = -(farPlane + nearPlane) / (farPlane - nearPlane);
			return m;
		}
	};

	using Matrix2x2 = Matrix<2, 2>;
	using Matrix3x3 = Matrix<3, 3>;
	using Matrix4x4 = Matrix<4, 4>;

	static_assert(std::is_trivially_copyable_v<Matrix4x4>);
	static_assert(sizeof(Matrix4x4) == sizeof(float) * 16);
}
