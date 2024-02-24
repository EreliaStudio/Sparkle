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
         * @param os The output stream.
         * @param p_quaternion The quaternion to output.
         * @return A reference to the output stream.
         */
        friend std::ostream& operator<<(std::ostream& p_os, const Quaternion& p_quaternion)
        {
            p_os << p_quaternion.x << " / " << p_quaternion.y << " / " << p_quaternion.z << " / " << p_quaternion.w;
            return (p_os);
        }

        Vector3 toVector3() const;
    };
}
