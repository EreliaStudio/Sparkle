#pragma once

#include "math/spk_vector3.hpp"
#include <cmath>
#include <iostream>

namespace spk
{
    /**
     * @class Quaternion
     * @brief Represents a quaternion for handling rotations in 3D space.
     * 
     * Quaternions are a system of complex numbers used in 3D computer graphics and game development
     * for representing rotations. They offer advantages over traditional Euler angles and rotation matrices
     * by being more compact, more numerically stable, and less susceptible to gimbal lock.
     */
    class Quaternion
    {
    public:
        float x; ///< The x component of the quaternion.
        float y; ///< The y component of the quaternion.
        float z; ///< The z component of the quaternion.
        float w; ///< The w (scalar) component of the quaternion.

        /**
         * @brief Default constructor, initializes the quaternion to the identity rotation.
         */
        Quaternion();

        /**
         * @brief Constructor from individual components.
         * @param p_x The x component.
         * @param p_y The y component.
         * @param p_z The z component.
         * @param p_w The w component.
         */
        Quaternion(const float& p_x, const float& p_y, const float& p_z, const float& p_w);

        /**
         * @brief Constructor from Euler angles.
         * @param p_eulerAngles A Vector3 representing the Euler angles in degrees.
         */
        Quaternion(const spk::Vector3& p_eulerAngles);

        /**
         * @brief Adds this quaternion to another quaternion.
         * 
         * This operation adds the corresponding components of two quaternions. It is useful for combining rotations
         * or interpolations between quaternions.
         *
         * @param rhs The quaternion to add to this quaternion.
         * @return A new quaternion that is the sum of this quaternion and the input quaternion.
         */
        Quaternion operator+(const Quaternion& rhs) const;

        /**
         * @brief Adds another quaternion to this quaternion and assigns the result to this quaternion.
         * 
         * This operation performs an in-place addition, updating this quaternion with the sum of itself and the input quaternion.
         *
         * @param rhs The quaternion to add to this quaternion.
         * @return A reference to this quaternion after the addition.
         */
        Quaternion& operator+=(const Quaternion& rhs);

        /**
         * @brief Subtracts another quaternion from this quaternion.
         * 
         * This operation subtracts the corresponding components of the input quaternion from this quaternion, useful
         * for finding the difference between two rotations.
         *
         * @param rhs The quaternion to subtract from this quaternion.
         * @return A new quaternion that is the result of the subtraction.
         */
        Quaternion operator-(const Quaternion& rhs) const;

        /**
         * @brief Subtracts another quaternion from this quaternion and assigns the result to this quaternion.
         * 
         * This operation performs an in-place subtraction, updating this quaternion with the result of the subtraction.
         *
         * @param rhs The quaternion to subtract from this quaternion.
         * @return A reference to this quaternion after the subtraction.
         */
        Quaternion& operator-=(const Quaternion& rhs);

        /**
         * @brief Multiplies this quaternion by a scalar.
         * 
         * This operation scales the components of the quaternion by a scalar value. It's useful for normalizing a quaternion
         * or adjusting its magnitude.
         *
         * @param scalar The scalar value to multiply the quaternion by.
         * @return A new quaternion that is the result of the multiplication.
         */
        Quaternion operator*(const float& scalar) const;

        /**
         * @brief Multiplies this quaternion by a scalar and assigns the result to this quaternion.
         * 
         * This operation performs an in-place multiplication, scaling this quaternion by the scalar value.
         *
         * @param scalar The scalar value to multiply this quaternion by.
         * @return A reference to this quaternion after the multiplication.
         */
        Quaternion& operator*=(const float& scalar);

        /**
         * @brief Divides this quaternion by a scalar.
         * 
         * This operation divides the components of the quaternion by a scalar value. It's useful for adjusting the
         * magnitude of a quaternion.
         *
         * @param scalar The scalar value to divide the quaternion by.
         * @return A new quaternion that is the result of the division.
         */
        Quaternion operator/(const float& scalar) const;

        /**
         * @brief Divides this quaternion by a scalar and assigns the result to this quaternion.
         * 
         * This operation performs an in-place division, scaling this quaternion by the reciprocal of the scalar value.
         *
         * @param scalar The scalar value to divide this quaternion by.
         * @return A reference to this quaternion after the division.
         */
        Quaternion& operator/=(const float& scalar);

        
        friend Quaternion operator*(const float& scalar, const Quaternion& q) {
            return (q * scalar);
        }
        
        friend Quaternion operator/(const float& scalar, const Quaternion& q) {
            return (q / scalar);
        }

        /**
         * @brief Checks if this quaternion is equal to another quaternion.
         * 
         * Equality is determined by comparing the corresponding components of the two quaternions.
         *
         * @param p_rhs The quaternion to compare with this quaternion.
         * @return true if the quaternions are equal, false otherwise.
         */
        bool operator==(const Quaternion& p_rhs) const;

        /**
         * @brief Checks if this quaternion is not equal to another quaternion.
         * 
         * Inequality is determined by comparing the corresponding components of the two quaternions.
         *
         * @param p_rhs The quaternion to compare with this quaternion.
         * @return true if the quaternions are not equal, false otherwise.
         */
        bool operator!=(const Quaternion& p_rhs) const;

        /**
         * @brief Less than operator for comparing two quaternions.
         * 
         * This comparison is based on a lexicographical order, primarily comparing the x component, then y, z, and w sequentially
         * if previous components are equal. This allows quaternions to be sorted or used in data structures requiring an ordering.
         *
         * @param p_rhs The quaternion to compare with this quaternion.
         * @return true if this quaternion is considered less than the other, false otherwise.
         */
        bool operator<(const Quaternion& p_rhs) const;

        /**
         * @brief Greater than operator for comparing two quaternions.
         * 
         * Utilizes the less than (<) and equality (==) operators to determine if this quaternion is greater than another.
         * This is essential for maintaining consistency in sorted data structures or when performing comparisons.
         *
         * @param p_rhs The quaternion to compare with this quaternion.
         * @return true if this quaternion is considered greater than the other, false otherwise.
         */
        bool operator>(const Quaternion& p_rhs) const;

        /**
         * @brief Less than or equal to operator for comparing two quaternions.
         * 
         * Determines if this quaternion is less than or equal to another by leveraging the greater than (>) operator.
         * It ensures a consistent ordering mechanism for quaternions within containers or comparison operations.
         *
         * @param p_rhs The quaternion to compare with this quaternion.
         * @return true if this quaternion is less than or equal to the other, false otherwise.
         */
        bool operator<=(const Quaternion& p_rhs) const;

        /**
         * @brief Greater than or equal to operator for comparing two quaternions.
         * 
         * Checks if this quaternion is greater than or equal to another by using the less than (<) operator.
         * This comparison is vital for data structures that depend on a defined ordering of elements.
         *
         * @param p_rhs The quaternion to compare with this quaternion.
         * @return true if this quaternion is greater than or equal to the other, false otherwise.
         */
        bool operator>=(const Quaternion& p_rhs) const;

        /**
         * @brief Normalizes the quaternion.
         * @return A new quaternion that is the normalized version of this quaternion.
         */
        Quaternion normalize() const;

        /**
         * @brief Calculates the inverse of the quaternion.
         * @return A new quaternion that is the inverse of this quaternion.
         */
        Quaternion inverse() const;

        /**
         * @brief Calculates the conjugate of the quaternion.
         * @return A new quaternion that is the conjugate of this quaternion.
         */
        Quaternion conjugate() const;

        /**
         * @brief Multiplies this quaternion by another quaternion.
         * @param p_rhs The right-hand side quaternion to multiply with.
         * @return A new quaternion that is the result of the multiplication.
         */
        Quaternion operator*(const Quaternion& p_rhs) const;

        /**
         * @brief Multiplies this quaternion by another quaternion and assigns the result to this quaternion.
         * @param p_rhs The right-hand side quaternion to multiply with.
         * @return A reference to this quaternion after the multiplication.
         */
        Quaternion& operator*=(const Quaternion& p_rhs);

        /**
         * @brief Creates a quaternion representing a rotation looking in a specific direction.
         * @param p_direction The direction to look at.
         * @param p_up The up vector, usually (0, 1, 0).
         * @return A quaternion representing the specified look-at rotation.
         */
        static Quaternion lookAt(const Vector3& p_direction, const Vector3& p_up);

        /**
         * @brief Creates a quaternion from Euler angles.
         * @param p_eulerAngles A Vector3 representing the Euler angles in degrees.
         * @return A quaternion representing the specified Euler angles rotation.
         */
        static Quaternion fromEulerAngles(const Vector3& p_eulerAngles);
        /**
         * @brief Creates a quaternion from Euler angles.
         * @param p_angleX A float representing the Euler angles around the X axis in degrees.
         * @param p_angleY A float representing the Euler angles around the Y axis in degrees.
         * @param p_angleZ A float representing the Euler angles around the Z axis in degrees.
         * @return A quaternion representing the specified Euler angles rotation.
         */
        static Quaternion fromEulerAngles(const float& p_angleX, const float& p_angleY, const float& p_angleZ);

		/**
         * @brief Creates a quaternion from an axis and an angle.
         * @param p_axis The axis of rotation.
         * @param p_angleDegree The angle of rotation in degrees.
         * @return A quaternion representing the rotation around the axis by the specified angle.
         */
        static Quaternion fromAxisAngle(const Vector3& p_axis, float p_angleDegree);

        /**
         * @brief Applies the rotation represented by this quaternion to a point.
         * @param p_inputPoint The point to rotate.
         * @return The rotated point.
         */
        Vector3 applyRotation(const spk::Vector3& p_inputPoint) const;

        /**
         * @brief Outputs the quaternion to an output stream in the format (x, y, z, w).
         * @param p_os The output stream.
         * @param p_quaternion The quaternion to output.
         * @return A reference to the output stream.
         */
        friend std::ostream& operator<<(std::ostream& p_os, const Quaternion& p_quaternion)
        {
            p_os << p_quaternion.x << " / " << p_quaternion.y << " / " << p_quaternion.z << " / " << p_quaternion.w;
            return (p_os);
        }

        /**
         * @brief Convert the quaternion into a spk::Vector3 using th x, y and z values of the original quaternion
         * 
         * @return The vector composed of the x, y and z attributes of the quaternion
        */
        Vector3 toVector3() const;
    };
}
