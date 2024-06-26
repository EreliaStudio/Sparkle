#pragma once

#include <cmath>

#include "math/spk_vector2.hpp"
#include "math/spk_vector3.hpp"

namespace spk
{
	/**
	 * @class Matrix4x4
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
	 *							spk::Matrix4x4::rotationMatrix({0.0f, 0.0f, 1.0f}) *
	 *							spk::Matrix4x4::scaleMatrix({1.0f, 2.0f, 3.0f});
	 * spk::Vector3 transformedPoint = transform * spk::Vector3{1.0f, 1.0f, 1.0f};
	 * @endcode
	 *
	 * @note The Matrix4x4 struct is designed for use in systems where performance and precision are crucial, such as real-time 3D rendering engines.
	 *
	 * @see IVector3
	 */
	struct Matrix4x4
	{
		/**
		 * @brief 4x4 matrix data stored in row-major order.
		 *
		 * This array represents the elements of the matrix, where each sub-array corresponds to a row in the matrix.
		 * The matrix is initialized to the identity matrix by default.
		 */
		float data[4][4] = {
			{1, 0, 0, 0},
			{0, 1, 0, 0},
			{0, 0, 1, 0},
			{0, 0, 0, 1}};

		/**
		 * @brief Default constructor for Matrix4x4.
		 *
		 * Initializes a Matrix4x4 to an identity matrix.
		 */
		Matrix4x4();

		/**
		 * @brief Predetermined constructor for Matrix4x4.
		 *
		 * Initializes a Matrix4x4 with specific values.
		 */
		Matrix4x4(float a1, float a2, float a3, float a4,  // First row
				  float b1, float b2, float b3, float b4,  // Second row
				  float c1, float c2, float c3, float c4,  // Third row
				  float d1, float d2, float d3, float d4); // Forth row

		/**
		 * @brief Add another matrix to this Matrix4x4.
		 *
		 * @param other The matrix to add.
		 * @return The result of matrix addition.
		 */
		Matrix4x4 operator+(const Matrix4x4& other) const;

		/**
		 * @brief Multiplies this matrix by a Vector3, applying the transformation.
		 *
		 * @param v The Vector3 to transform.
		 * @return A Vector3 that has been transformed by this matrix.
		 */
		Vector3 operator*(const Vector3& v) const;

		/**
		 * @brief Multiplies this matrix by a Vector2, applying the transformation.
		 *
		 * Assumes the z-coordinate is 0. Useful for transformations in 2D space.
		 *
		 * @param v The Vector2 to transform.
		 * @return A Vector2 that has been transformed by this matrix.
		 */
		Vector2 operator*(const Vector2& v) const;

		/**
		 * @brief Multiplies this matrix by another Matrix4x4.
		 *
		 * @param other The matrix to multiply with.
		 * @return The result of matrix multiplication.
		 */
		Matrix4x4 operator*(const Matrix4x4& other) const;

		/**
		 * @brief Creates a lookAt matrix for camera transformations.
		 *
		 * @param p_from The position of the camera.
		 * @param p_to The target the camera is looking at.
		 * @param p_up The up direction for the camera.
		 * @return A Matrix4x4 representing the lookAt transformation.
		 */
		static Matrix4x4 lookAt(const Vector3& p_from, const Vector3& p_to, const Vector3& p_up = Vector3(0, 1, 0));

		/**
		 * @brief Creates a translation matrix.
		 *
		 * @param p_translation The translation vector.
		 * @return A Matrix4x4 representing the translation.
		 */
		static Matrix4x4 translationMatrix(const Vector3& p_translation);

		/**
		 * @brief Creates a scale matrix.
		 *
		 * @param p_scale The scaling factors.
		 * @return A Matrix4x4 representing the scaling.
		 */
		static Matrix4x4 scaleMatrix(const Vector3& p_scale);

		/**
		 * @brief Creates a rotation matrix.
		 *
		 * @param p_rotation The rotation angles in degree (pitch, yaw, roll).
		 * @return A Matrix4x4 representing the rotation.
		 */
		static Matrix4x4 rotationMatrix(const Vector3& p_rotation);

		/**
		 * @brief Creates a rotation matrix around a given axis.
		 *
		 * @param p_axis The axis to rotate around.
		 * @param p_rotationAngle The angle of rotation in degree.
		 * @return A Matrix4x4 representing the rotation around the axis.
		 */
		static Matrix4x4 rotateAroundAxis(const Vector3& p_axis, const float& p_rotationAngle);

		/**
		 * @brief Creates a perspective projection matrix.
		 *
		 * @param fov The field of view in degree.
		 * @param aspectRatio The aspect ratio of the viewport.
		 * @param nearPlane The distance to the near clipping plane.
		 * @param farPlane The distance to the far clipping plane.
		 * @return A Matrix4x4 representing the perspective projection.
		 */
		static Matrix4x4 perspective(float fov, float aspectRatio, float nearPlane, float farPlane);

		/**
		 * @brief Creates an orthographic projection matrix.
		 *
		 * @param left The left boundary of the clipping volume.
		 * @param right The right boundary of the clipping volume.
		 * @param bottom The bottom boundary of the clipping volume.
		 * @param top The top boundary of the clipping volume.
		 * @param nearPlane The distance to the near clipping plane.
		 * @param farPlane The distance to the far clipping plane.
		 * @return A Matrix4x4 representing the orthographic projection.
		 */
		static Matrix4x4 ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane);

		/**
		 * @brief Checks if this matrix is equal to another matrix.
		 *
		 * @param other The matrix to compare with.
		 * @return true if the matrices are equal, false otherwise.
		 */
		bool operator==(const Matrix4x4& other) const;

		/**
		 * @brief Checks if this matrix is not equal to another matrix.
		 *
		 * @param other The matrix to compare with.
		 * @return true if the matrices are not equal, false otherwise.
		 */
		bool operator!=(const Matrix4x4& other) const;

		/**
		 * @brief Calculates the transpose of this matrix.
		 *
		 * @return A new Matrix4x4 that is the transpose of this matrix.
		 */
		Matrix4x4 transpose() const;

		/**
		 * @brief Calculates the determinant of a 2x2 matrix.
		 *
		 * @param a The top-left element.
		 * @param b The top-right element.
		 * @param c The bottom-left element.
		 * @param d The bottom-right element.
		 * @return The determinant of the 2x2 matrix.
		 */
		float determinant2x2(float a, float b, float c, float d);

		/**
		 * @brief Calculates the inverse of this matrix.
		 *
		 * @return A new Matrix4x4 that is the inverse of this matrix.
		 */
		Matrix4x4 inverse() const;

		/**
		 * @brief Outputs the contents of the matrix to a ostream.
		 *
		 * This operator overload allows for easy printing of the matrix to output streams (e.g., cout).
		 * It iterates through the matrix rows and columns, inserting each element into the stream,
		 * formatted in a 4x4 matrix layout for readability.
		 *
		 * @param p_os The output stream to which the matrix data will be sent.
		 * @param p_matrix The Matrix4x4 instance to output.
		 * @return A reference to the output stream to support chaining of output operations.
		 */
		friend std::ostream& operator<<(std::ostream& p_os, const Matrix4x4& p_matrix)
		{
			p_os << std::endl;
			for (size_t y = 0; y < 4; y++)
			{
				for (size_t x = 0; x < 4; x++)
				{
					if (x != 0)
						p_os << " ";
					p_os << std::setw(10) << p_matrix.data[x][y];
				}
				p_os << std::endl;
			}

			return p_os;
		}

		/**
		 * @brief Returns a pointer to the first element of the matrix data array.
		 *
		 * Useful for passing the matrix to graphics APIs like OpenGL.
		 *
		 * @return A pointer to the matrix data.
		 */
		float* ptr();

		/**
		 * @brief Returns a constant pointer to the first element of the matrix data array.
		 *
		 * Useful for passing the matrix to graphics APIs like OpenGL in a const context.
		 *
		 * @return A constant pointer to the matrix data.
		 */
		const float* c_ptr() const;

	};
}
