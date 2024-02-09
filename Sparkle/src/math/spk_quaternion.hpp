#pragma once

#include "math/spk_vector3.hpp"
#include <cmath>
#include <iostream>

namespace spk
{
    class Quaternion
    {
    public:
        float x;
        float y;
        float z;
        float w;

        Quaternion();
        Quaternion(const float& p_x, const float& p_y, const float& p_z, const float& p_w);
        Quaternion(const spk::Vector3& p_eulerAngles);

        Quaternion normalize() const;
        Quaternion inverse() const;
        Quaternion conjugate() const;

        Quaternion operator*(const Quaternion& p_rhs) const;
        Quaternion& operator*=(const Quaternion& p_rhs);

        static Quaternion lookAt(const Vector3& p_direction, const Vector3& p_up);
        static Quaternion fromEulerAngles(const Vector3& p_eulerAngles);
		static Quaternion fromAxisAngle(const Vector3& p_axis, float p_angleDegree);
    
        Vector3 applyRotation(const spk::Vector3& p_inputPoint) const;

        friend std::ostream& operator<<(std::ostream& os, const Quaternion& p_quaternion)
        {
            os << "(" << p_quaternion.x << ", " << p_quaternion.y << ", " << p_quaternion.z << ", " << p_quaternion.w << ")";
            return os;
        }
    };
}
