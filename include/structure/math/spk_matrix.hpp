#pragma once

#include <stdexcept>
#include <string>

#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"

namespace spk
{
	template <size_t SizeX, size_t SizeY>
	class IMatrix
	{
	public:
		class Column
		{
		private:
			float rows[SizeY];

		public:
			float& operator[](size_t p_index)
			{
				if (p_index >= SizeY)
					throw std::invalid_argument("Can't access the row " + std::to_string(p_index) + " on a matrix " + std::to_string(SizeX) + "x" + std::to_string(SizeY));
				return (rows[p_index]);
			}
			const float& operator[](size_t p_index) const
			{
				if (p_index >= SizeY)
					throw std::invalid_argument("Can't access the row " + std::to_string(p_index) + " on a matrix " + std::to_string(SizeX) + "x" + std::to_string(SizeY));
				return (rows[p_index]);
			}
		};

	private:
		Column cols[SizeX];

	public:
		IMatrix()
		{
			for (size_t i = 0; i < SizeX; i++)
			{
				for (size_t j = 0; j < SizeY; j++)
				{
					(*this)[i][j] = (i == j ? 1.0f : 0.0f);
				}
			}
		}

		IMatrix(float* p_values)
		{
			for (size_t i = 0; i < SizeX; i++)
			{
				for (size_t j = 0; j < SizeY; j++)
				{
					(*this)[i][j] = p_values[i * SizeY + j];
				}
			}
		}

		IMatrix(std::initializer_list<float> values)
		{
			if (values.size() != SizeX * SizeY)
			{
				throw std::invalid_argument("Initializer list size does not match the matrix dimensions.");
			}

			auto it = values.begin();
			for (size_t j = 0; j < SizeY; j++)
			{
				for (size_t i = 0; i < SizeX; i++)
				{
					(*this)[i][j] = *it++;
				}
			}
		}

		Column& operator[](size_t p_index)
		{
			if (p_index >= SizeX)
				throw std::invalid_argument("Can't access the column " + std::to_string(p_index) + " on a matrix " + std::to_string(SizeX) + "x" + std::to_string(SizeY));
			return (cols[p_index]);
		}

		const Column& operator[](size_t p_index) const
		{
			if (p_index >= SizeX)
				throw std::invalid_argument("Can't access the column " + std::to_string(p_index) + " on a matrix " + std::to_string(SizeX) + "x" + std::to_string(SizeY));
			return (cols[p_index]);
		}

		Vector3 operator*(const Vector3& p_vec) const
		{
			if (SizeX != 4 || SizeY != 4)
			{
				throw std::invalid_argument("Matrix must be 4x4 for this operation.");
			}

			Vector3 result;
			result.x = (*this)[0][0] * p_vec.x + (*this)[1][0] * p_vec.y + (*this)[2][0] * p_vec.z + (*this)[3][0] * 1;
			result.y = (*this)[0][1] * p_vec.x + (*this)[1][1] * p_vec.y + (*this)[2][1] * p_vec.z + (*this)[3][1] * 1;
			result.z = (*this)[0][2] * p_vec.x + (*this)[1][2] * p_vec.y + (*this)[2][2] * p_vec.z + (*this)[3][2] * 1;

			return result;
		}

		Vector2 operator*(const Vector2& p_vec) const
		{
			if (SizeX != 3 || SizeY != 3)
			{
				throw std::invalid_argument("Matrix must be 3x3 for this operation.");
			}

			Vector2 result;
			result.x = (*this)[0][0] * p_vec.x + (*this)[1][0] * p_vec.y + (*this)[2][0] * 1;
			result.y = (*this)[0][1] * p_vec.x + (*this)[1][1] * p_vec.y + (*this)[2][1] * 1;

			return result;
		}

		IMatrix<SizeX, SizeY> operator*(const IMatrix<SizeX, SizeY>& p_other) const
		{
			IMatrix<SizeX, SizeY> result;

			for (size_t i = 0; i < SizeX; ++i)
			{
				for (size_t j = 0; j < SizeY; ++j)
				{
					result[i][j] = 0;
					for (size_t k = 0; k < SizeX; ++k)
					{
						result[i][j] += (*this)[i][k] * p_other[k][j];
					}
				}
			}

			return result;
		}

		bool operator==(const IMatrix& p_other) const
		{
			for (size_t i = 0; i < SizeX; ++i)
			{
				for (size_t j = 0; j < SizeY; ++j)
				{
					if ((*this)[i][j] != p_other[i][j])
					{
						return false;
					}
				}
			}
			return true;
		}

		bool operator!=(const IMatrix& p_other) const
		{
			return !(*this == p_other);
		}

		static IMatrix<4, 4> rotationMatrix(float p_angleX, float p_angleY, float p_angleZ)
		{
			IMatrix<4, 4> mat;

			float cosX = std::cos(spk::degreeToRadian(p_angleX));
			float sinX = std::sin(spk::degreeToRadian(p_angleX));

			float cosY = std::cos(spk::degreeToRadian(p_angleY));
			float sinY = std::sin(spk::degreeToRadian(p_angleY));

			float cosZ = std::cos(spk::degreeToRadian(p_angleZ));
			float sinZ = std::sin(spk::degreeToRadian(p_angleZ));

			IMatrix<4, 4> rotationX = {
				1, 0, 0, 0,
				0, cosX, -sinX, 0,
				0, sinX, cosX, 0,
				0, 0, 0, 1
			};

			IMatrix<4, 4> rotationY = {
				cosY, 0, sinY, 0,
				0, 1, 0, 0,
				-sinY, 0, cosY, 0,
				0, 0, 0, 1
			};

			IMatrix<4, 4> rotationZ = {
				cosZ, -sinZ, 0, 0,
				sinZ, cosZ, 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1
			};

			IMatrix<4, 4> rotation = rotationZ * rotationY * rotationX;

			return rotation;
		}

		static IMatrix<4, 4> rotationMatrix(spk::Vector3 p_angle)
		{
			return (rotationMatrix(p_angle.x, p_angle.y, p_angle.z));
		}

		static IMatrix<4, 4> translationMatrix(float p_translateX, float p_translateY, float p_translateZ)
		{
			IMatrix<4, 4> mat = {
				1, 0, 0, p_translateX,
				0, 1, 0, p_translateY,
				0, 0, 1, p_translateZ,
				0, 0, 0, 1
			};

			return mat;
		}

		static IMatrix<4, 4> translationMatrix(spk::Vector3 p_translation)
		{
			return (translationMatrix(p_translation.x, p_translation.y, p_translation.z));
		}

		// Scale Matrix
		static IMatrix<4, 4> scaleMatrix(float p_scaleX, float p_scaleY, float p_scaleZ)
		{
			IMatrix<4, 4> mat = {
				p_scaleX, 0, 0, 0,
				0, p_scaleY, 0, 0,
				0, 0, p_scaleZ, 0,
				0, 0, 0, 1
			};

			return mat;
		}

		static IMatrix<4, 4> scaleMatrix(spk::Vector3 p_scale)
		{
			return (scaleMatrix(p_scale.x, p_scale.y, p_scale.z));
		}

		friend std::wostream& operator<<(std::wostream& p_os, const IMatrix& p_mat)
		{
			for (size_t j = 0; j < SizeY; ++j)
			{
				if (j != 0)
					p_os << " - ";

				p_os << L"[";
				for (size_t i = 0; i < SizeX; ++i)
				{
					if (i != 0)
						p_os << L" - ";
					p_os << p_mat[i][j];
				}
				p_os << L"]";
			}
			return p_os;
		}

		friend std::ostream& operator<<(std::ostream& p_os, const IMatrix& p_mat)
		{
			for (size_t j = 0; j < SizeY; ++j)
			{
				if (j != 0)
					p_os << " - ";

				p_os << "[";
				for (size_t i = 0; i < SizeX; ++i)
				{
					if (i != 0)
						p_os << " - ";
					p_os << p_mat[i][j];
				}
				p_os << "]";
			}
			return p_os;
		}

		std::wstring to_wstring() const
		{
			std::wstringstream wss;
			wss << *this;
			return wss.str();
		}

		std::string to_string() const
		{
			std::stringstream ss;
			ss << *this;
			return ss.str();
		}

		template <size_t X = SizeX, size_t Y = SizeY, typename std::enable_if_t<(X == 4 && Y == 4), int> = 0>
		static IMatrix lookAt(const spk::Vector3& p_from, const spk::Vector3& p_to, const spk::Vector3& p_up)
		{
			const spk::Vector3 forward = ((p_to - p_from).normalize());
			const spk::Vector3 right = (forward != p_up && forward != p_up.inverse() ? forward.cross(p_up).normalize() : spk::Vector3(1, 0, 0));
			const spk::Vector3 up = right.cross(forward);

			IMatrix result;

			result[0][0] = right.x;
			result[1][0] = right.y;
			result[2][0] = right.z;
			result[1][0] = up.x;
			result[1][1] = up.y;
			result[1][2] = up.z;
			result[2][0] = -forward.x;
			result[2][1] = -forward.y;
			result[2][2] = -forward.z;
			result[3][0] = -right.dot(p_from);
			result[3][1] = -up.dot(p_from);
			result[3][2] = forward.dot(p_from);

			return result;
		}

		template <size_t X = SizeX, size_t Y = SizeY, typename std::enable_if_t<(X == 4 && Y == 4), int> = 0>
		static IMatrix rotateAroundAxis(const spk::Vector3& p_axis, const float& p_rotationAngle)
		{
			IMatrix result;

			const float rad = spk::degreeToRadian(p_rotationAngle);

			Vector3 v = p_axis.normalize();

			float c = cos(rad);
			float s = sin(rad);

			result[0][0] = c + v.x * v.x * (1 - c);
			result[0][1] = v.x * v.y * (1 - c) - v.z * s;
			result[0][2] = v.x * v.z * (1 - c) + v.y * s;

			result[1][0] = v.y * v.x * (1 - c) + v.z * s;
			result[1][1] = c + v.y * v.y * (1 - c);
			result[1][2] = v.y * v.z * (1 - c) - v.x * s;

			result[2][0] = v.z * v.x * (1 - c) - v.y * s;
			result[2][1] = v.z * v.y * (1 - c) + v.x * s;
			result[2][2] = c + v.z * v.z * (1 - c);

			result[3][0] = 0.0f;
			result[3][1] = 0.0f;
			result[3][2] = 0.0f;
			result[3][3] = 1.0f;

			return result;
		}

		template <size_t X = SizeX, size_t Y = SizeY, typename std::enable_if_t<(X == 4 && Y == 4), int> = 0>
		static IMatrix perspective(float p_fov, float p_aspectRatio, float p_nearPlane, float p_farPlane)
		{
			IMatrix result;

			const float rad = spk::degreeToRadian(p_fov);
			const float tanHalfFov = tan(rad / 2.0f);

			result[0][0] = 1.0f / (tanHalfFov * p_aspectRatio);
			result[1][1] = -1.0f / tanHalfFov;
			result[2][2] = -(p_farPlane + p_nearPlane) / (p_farPlane - p_nearPlane);
			result[3][2] = -1.0f;
			result[2][3] = -(2.0f * p_farPlane * p_nearPlane) / (p_farPlane - p_nearPlane);
			result[3][3] = 0.0f;

			return result;
		}

		template <size_t X = SizeX, size_t Y = SizeY, typename std::enable_if_t<(X == 4 && Y == 4), int> = 0>
		static IMatrix ortho(float p_left, float p_right, float p_bottom, float p_top, float p_nearPlane, float p_farPlane)
		{
			IMatrix result;

			result[0][0] = 2.0f / (p_right - p_left);
			result[1][1] = 2.0f / (p_top - p_bottom);
			result[2][2] = -2.0f / (p_farPlane - p_nearPlane);
			result[3][0] = -(p_right + p_left) / (p_right - p_left);
			result[3][1] = -(p_top + p_bottom) / (p_top - p_bottom);
			result[3][2] = -(p_farPlane + p_nearPlane) / (p_farPlane - p_nearPlane);

			return result;
		}
	};

	using Matrix2x2 = IMatrix<3, 3>;
	using Matrix3x3 = IMatrix<3, 3>;
	using Matrix4x4 = IMatrix<4, 4>;
}