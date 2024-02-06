#pragma once

#include <cmath>

#include "math/spk_vector2.hpp"
#include "math/spk_vector3.hpp"

namespace spk
{
    /**
     * @brief Represents a 4x4 matrix and provides a comprehensive set of operations for graphical
     * transformations.
     *
     * This struct encapsulates a 4x4 matrix, offering basic matrix operations such as multiplication,
     * inversion, and transposition, along with more specific transformation functions like translation,
     * scaling, and rotation.
     * 
     * It is essential for 3D graphics programming, facilitating transformations in 3D space, including
     * object positioning, scaling, rotation, and camera setups for perspective and orthogonal projections.
     *
     * The Matrix4x4 struct also defines static methods to create transformation matrices for common
     * operations (e.g., lookAt, perspective, ortho) and supports direct multiplication with Vector3 to
     * apply transformations.
     *
     * Usage example:
     * @code
     * spk::Matrix4x4 transform = spk::Matrix4x4::translationMatrix({1.0f, 2.0f, 3.0f}) *
     *                            spk::Matrix4x4::rotationMatrix({0.0f, 0.0f, 1.0f}) *
     *                            spk::Matrix4x4::scaleMatrix({1.0f, 2.0f, 3.0f});
     * spk::Vector3 transformedPoint = transform * spk::Vector3{1.0f, 1.0f, 1.0f};
     * @endcode
     *
     * @note The Matrix4x4 struct is designed for use in systems where performance and precision are crucial, such as real-time 3D rendering engines.
     * 
     * @see IVector3
     */
	struct Matrix4x4
    {
        float data[4][4] = {
			{1, 0, 0, 0},
			{0, 1, 0, 0},
			{0, 0, 1, 0},
			{0, 0, 0, 1}
		};

		Matrix4x4();

        friend std::wostream& operator << (std::wostream& p_os, const Matrix4x4& p_matrix)
        {
            p_os << std::endl;
            for (size_t i = 0; i < 4; i++)
            {
                for (size_t j = 0; j < 4; j++)
                {
                    if (j != 0)
                        p_os << " ";
                    p_os << p_matrix.data[i][j];
                }
                p_os << std::endl;
            }
            
            return (p_os);
        }

        Vector3 operator*(const Vector3& v) const;
        Vector2 operator*(const Vector2& v) const;

        Matrix4x4 operator*(const Matrix4x4& other) const;

		static Matrix4x4 lookAt(const Vector3& p_from, const Vector3& p_to, const Vector3& p_up = Vector3(0, 1, 0));

        static Matrix4x4 translationMatrix(const Vector3& p_translation);
        static Matrix4x4 scaleMatrix(const Vector3& p_scale);
        static Matrix4x4 rotationMatrix(const Vector3& p_rotation);

		static Matrix4x4 rotateAroundAxis(const Vector3& p_axis, const float& p_rotationAngle);

        static Matrix4x4 perspective(float fov, float aspectRatio, float nearPlane, float farPlane);
        static Matrix4x4 ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane);

		bool operator==(const Matrix4x4& other) const;
		bool operator!=(const Matrix4x4& other) const;

		Matrix4x4 transpose() const;

		float determinant2x2(float a, float b, float c, float d);

		Matrix4x4 inverse() const;

        friend std::ostream& operator << (std::ostream& p_os, const Matrix4x4& p_matrix)
        {
            p_os << std::endl;
            for (size_t i = 0; i < 4; i++)
            {
                for (size_t j = 0; j < 4; j++)
                {
                    if (j != 0)
                        p_os << " ";
                    p_os << p_matrix.data[i][j];
                }
                p_os << std::endl;
            }

            return p_os;
        }

		float* ptr();
		const float* c_ptr() const;
    };
}
