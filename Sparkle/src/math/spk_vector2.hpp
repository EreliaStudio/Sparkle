#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>

#include <algorithm>
#include <iostream>
#include <string>

#include "spk_basic_functions.hpp"

#include "miscellaneous/JSON/spk_JSON_object.hpp"

namespace spk
{
    /**
     * @interface IVector2
     *
     * @tparam TType The numeric type used to represent the vector's components. It must be an arithmetic type (e.g., int, float, double).
     *
     * @brief A 2D vector template class for storing and manipulating 2D point coordinates.
     *
     * This template class provides functionality for working with 2D vectors, commonly
     * used for representing 2D point coordinates or directions in space.
     * It supports various arithmetic and geometric operations, including addition,
     * subtraction, dot product, normalization, and more.
     *
     * Usage example:
     * @code
     * spk::IVector2<float> point(3.0f, 4.0f);
     * float length = point.norm(); // Calculate the length of the vector
     * spk::IVector2<float> normalized = point.normalize(); // Normalize the vector
     * @endcode
     *
     */
    template <typename TType, typename = typename std::enable_if<std::is_arithmetic<TType>::value>::type>
    struct IVector2
    {
        TType x; //!< X component of the vector.
        TType y; //!< Y component of the vector.

        /**
         * @brief Default constructor. Initializes x and y to zero.
         */
        IVector2() :
            x(0),
            y(0)
        {
        }

        /**
         * @brief Constructs a vector with both x and y set to the provided value.
         * @param p_value The value to set both x and y components to.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        IVector2(const TOtherType& p_value) :
            x(static_cast<TType>(p_value)),
            y(static_cast<TType>(p_value))
        {
        }

        /**
         * @brief Constructs a vector from separate x and y values.
         * @param p_x The value for the x component.
         * @param p_y The value for the y component.
         */
        template <typename TTypeA, typename TTypeB, typename = typename std::enable_if<std::is_arithmetic<TTypeA>::value>::type, typename = typename std::enable_if<std::is_arithmetic<TTypeB>::value>::type>
        IVector2(const TTypeA& p_x, const TTypeB& p_y) :
            x(static_cast<TType>(p_x)),
            y(static_cast<TType>(p_y))
        {
        }

        /**
         * @brief Copy constructor from another vector of possibly different type.
         * @param p_other The vector to copy from.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        IVector2(const IVector2<TOtherType>& p_other) :
            x(static_cast<TType>(p_other.x)),
            y(static_cast<TType>(p_other.y))
        {
        }

        /**
         * @brief Conversion operator to another vector of possibly different type.
         * @return A new vector of the target type with copied components.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        operator IVector2<TOtherType>()
        {
            return (IVector2<TOtherType>(static_cast<TOtherType>(x), static_cast<TOtherType>(y)));
        }

        /**
         * @brief Unary negation operator.
         * @return A new vector where both components are negated.
         */
        IVector2 operator-() const
        {
            return (IVector2(-x, -y));
        }

        /**
         * @brief Converts the vector to a string representation.
         * @return A string in the format "x / y".
         */
        std::string to_string() const
        {
            return ("(" + std::to_string(x) + ", " + std::to_string(y) + ")");
        }

        /**
         * @brief Stream insertion operator to output the vector to an output stream.
         * @param p_os The output stream.
         * @param p_self The vector to output.
         * @return The output stream after insertion.
         */
        friend std::ostream& operator<<(std::ostream& p_os, const IVector2& p_self)
        {
            p_os << p_self.to_string();
            return p_os;
        }

        /**
         * @brief Adds two vectors component-wise.
         *
         * @param p_other Another vector to add to this vector.
         * @return The resultant vector after addition.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        IVector2<TType> operator+(const IVector2<TOtherType>& p_other) const
        {
            return IVector2<TType>(x + p_other.x, y + p_other.y);
        }

        /**
         * @brief Adds a scalar value to both components of the vector.
         *
         * @param p_value The scalar value to add.
         * @return The resultant vector after addition.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        IVector2<TType> operator+(const TOtherType& p_value) const
        {
            return IVector2<TType>(x + p_value, y + p_value);
        }

        /**
         * @brief Subtracts another vector from this vector component-wise.
         *
         * @param p_other The vector to subtract from this vector.
         * @return The resultant vector after subtraction.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        IVector2<TType> operator-(const IVector2<TOtherType>& p_other) const
        {
            return IVector2<TType>(x - p_other.x, y - p_other.y);
        }

        /**
         * @brief Subtracts a scalar value from both components of the vector.
         *
         * @param p_value The scalar value to subtract.
         * @return The resultant vector after subtraction.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        IVector2<TType> operator-(const TOtherType& p_value) const
        {
            return IVector2<TType>(x - p_value, y - p_value);
        }

        /**
         * @brief Multiplies this vector by another vector component-wise.
         *
         * @param p_other The vector to multiply with.
         * @return The resultant vector after multiplication.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        IVector2<TType> operator*(const IVector2<TOtherType>& p_other) const
        {
            return (IVector2<TType>(x * p_other.x, y * p_other.y));
        }

        /**
         * @brief Multiplies both components of this vector by a scalar value.
         *
         * @param p_value The scalar value to multiply with.
         * @return The resultant vector after multiplication.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        IVector2<TType> operator*(const TOtherType& p_value) const
        {
            return (IVector2<TType>(x * p_value, y * p_value));
        }

        /**
         * @brief Divides this vector by another vector component-wise.
         *
         * @param p_other The vector to divide by.
         * @return The resultant vector after division.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        IVector2<TType> operator/(const IVector2<TOtherType>& p_other) const
        {
            if (p_other.x == 0 || p_other.y == 0)
                throwException("Trying to divide by 0");
            return (IVector2<TType>(x / p_other.x, y / p_other.y));
        }

        /**
         * @brief Divides both components of this vector by a scalar value.
         *
         * @param p_value The scalar value to divide by.
         * @return The resultant vector after division.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        IVector2<TType> operator/(const TOtherType& p_value) const
        {
            if (p_value == 0)
                throwException("Trying to divide by 0");
            return (IVector2<TType>(x / p_value, y / p_value));
        }

        /**
         * @brief Adds another vector to this vector component-wise and assigns the result to this vector.
         *
         * @param p_other The vector to add to this vector.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        void operator+=(const IVector2<TOtherType>& p_other)
        {
            x += p_other.x;
            y += p_other.y;
        }

        /**
         * @brief Subtracts another vector from this vector component-wise and assigns the result to this vector.
         *
         * @param p_other The vector to subtract from this vector.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        void operator-=(const IVector2<TOtherType>& p_other)
        {
            x -= p_other.x;
            y -= p_other.y;
        }

        /**
         * @brief Multiplies this vector by another vector component-wise and assigns the result to this vector.
         *
         * @param p_other The vector to multiply this vector by.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        void operator*=(const IVector2<TOtherType>& p_other)
        {
            x *= p_other.x;
            y *= p_other.y;
        }

        /**
         * @brief Divides this vector by another vector component-wise and assigns the result to this vector.
         *
         * Checks for division by zero and throws an exception if attempted.
         *
         * @param p_other The vector to divide this vector by.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        void operator/=(const IVector2<TOtherType>& p_other)
        {
            if (p_other.x == 0 || p_other.y == 0)
                throwException("Trying to divide by 0");
            x /= p_other.x;
            y /= p_other.y;
        }

        /**
         * @brief Checks for equality between this vector and another vector.
         *
         * @param p_other The vector to compare with.
         * @return True if both x and y components are equal, false otherwise.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        bool operator==(const IVector2<TOtherType> &p_other) const
        {
            return (x == p_other.x && y == p_other.y);
        }

        /**
         * @brief Checks for equality between this vector and a scalar value.
         *
         * @param p_value The scalar value to compare with both components of the vector.
         * @return True if both components of the vector are equal to the scalar value, false otherwise.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        bool operator==(const TOtherType &p_value) const
        {
            return (x == p_value && y == p_value);
        }

        /**
         * @brief Checks for inequality between this vector and another vector.
         *
         * @param p_other The vector to compare with.
         * @return True if either x or y component is not equal, false otherwise.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        bool operator!=(const IVector2<TOtherType> &p_other) const
        {
            return (x != p_other.x || y != p_other.y);
        }

        /**
         * @brief Checks for inequality between this vector and a scalar value.
         *
         * @param p_value The scalar value to compare with both components of the vector.
         * @return True if either component of the vector is not equal to the scalar value, false otherwise.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        bool operator!=(const TOtherType &p_value) const
        {
            return (x != p_value || y != p_value);
        }

        /**
         * @brief Checks if this vector is less than another vector in a lexicographical manner.
         *
         * @param p_other The vector to compare with.
         * @return True if this vector is lexicographically less than the other vector, false otherwise.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        bool operator<(const IVector2<TOtherType> &p_other) const
        {
            return (y < p_other.y) || (y == p_other.y && x < p_other.x);
        }

        /**
         * @brief Checks if this vector is greater than another vector in a lexicographical manner.
         *
         * @param p_other The vector to compare with.
         * @return True if this vector is lexicographically greater than the other vector, false otherwise.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        bool operator>(const IVector2<TOtherType> &p_other) const
        {
            return (y > p_other.y) || (y == p_other.y && x > p_other.x);
        }

        /**
         * @brief Checks if this vector is less than or equal to another vector in a lexicographical manner.
         *
         * @param p_other The vector to compare with.
         * @return True if this vector is lexicographically less than or equal to the other vector, false otherwise.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        bool operator<=(const IVector2<TOtherType> &p_other) const
        {
            return (y < p_other.y) || (y == p_other.y && x <= p_other.x);
        }

        /**
         * @brief Checks if this vector is greater than or equal to another vector in a lexicographical manner.
         *
         * @param p_other The vector to compare with.
         * @return True if this vector is lexicographically greater than or equal to the other vector, false otherwise.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        bool operator>=(const IVector2<TOtherType> &p_other) const
        {
            return (y > p_other.y) || (y == p_other.y && x >= p_other.x);
        }

        /**
         * @brief Computes the Euclidean norm (length) of the vector.
         * @return The length of the vector.
         */
        float norm() const
        {
            return std::sqrt(squaredNorm());
        }

        /**
         * @brief Computes the squared Euclidean norm of the vector.
         * @return The squared length of the vector.
         */
        float squaredNorm() const
        {
            return x * x + y * y;
        }

        /**
         * @brief Computes the distance between this vector and another vector.
         * @param p_other The other vector.
         * @return The Euclidean distance between the two vectors.
         */
        float distance(const IVector2& p_other) const
        {
            return std::sqrt(distanceSquared(p_other));
        }

        /**
         * @brief Computes the distance between this vector and another vector.
         * @param p_other The other vector.
         * @return The Euclidean distance between the two vectors.
         */
        float distanceSquared(const IVector2& p_other) const
        {
            return static_cast<float>((p_other.x - x) * (p_other.x - x) + (p_other.y - y) * (p_other.y - y));
        }

        /**
         * @brief Normalizes the vector to unit length.
         * @return A new vector of float type with unit length in the same direction.
         */
        IVector2<float> normalize() const
        {
            float length = norm();

            if (length == 0)
            {
                throw std::runtime_error("Attempt to normalize a zero-length vector.");
            }

            return IVector2<float>(x / length, y / length);
        }

        /**
         * @brief Calculates the cross product of this vector and another vector, resulting in a vector orthogonal to both.
         *
         * Note: Since this is a 2D vector, the 'cross product' is simulated and results in a vector orthogonal to the plane.
         *
         * @param p_other The other vector involved in the cross product operation.
         * @return A new vector that is orthogonal to both this vector and `p_other`.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        IVector2 cross(const IVector2<TOtherType>& p_other = IVector2(0, 0)) const
        {
            IVector2 result;
            result = IVector2(-(p_other.y - y), p_other.x - x);
            return (result);
        }

        /**
         * @brief Calculates the Z component of the cross product of this vector and another vector.
         *
         * This method is useful in 2D space where the cross product results in a scalar value representing the magnitude in the Z direction.
         *
         * @param p_other The other vector involved in the cross product operation.
         * @return The scalar Z component of the cross product.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        float crossZ(const IVector2<TOtherType>& p_other) const
        {
            return (x * p_other.y - y * p_other.x);
        }

        /**
         * @brief Calculates the dot product of this vector and another vector, optionally considering a specific center point.
         *
         * @param p_other The other vector involved in the dot product operation.
         * @param p_center The point considered as the origin for the vectors (default is the origin of the coordinate system).
         * @return The scalar result of the dot product.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        float dot(const IVector2<TOtherType>& p_other, const IVector2<TOtherType>& p_center = IVector2(0, 0)) const
        {
            return (x - p_center.x) * (p_other.x - p_center.x) + (y - p_center.y) * (p_other.y - p_center.y);
        }

        /**
         * @brief Calculates the angle between this vector and another vector, optionally considering a specific center point.
         *
         * @param p_other The other vector to calculate the angle with.
         * @param p_center The point considered as the origin for the vectors (default is the origin of the coordinate system).
         * @return The angle between the two vectors in degrees.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        float angle(const IVector2<TOtherType>& p_other, const IVector2<TOtherType>& p_center = IVector2(0, 0)) const
        {
            float rdot = dot(p_other, p_center);
            rdot = std::clamp(rdot, -1.0f, 1.0f);
            return spk::radianToDegree(std::acos(rdot));
        }

        /**
         * @brief Rotates this vector around a given center point by a specified angle.
         *
         * @param p_center The center point around which to rotate the vector.
         * @param p_angle The angle of rotation in degrees.
         * @return A new vector that is the result of rotating this vector around `p_center` by `p_angle`.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        IVector2 rotate(const IVector2<TOtherType>& p_center, const float& p_angle) const
        {
            float theta = spk::degreeToRadian(p_angle);
            float pcos = std::cos(theta);
            float psin = std::sin(theta);
            IVector2 result;
            result.x = (x - p_center.x) * pcos - (y - p_center.y) * psin;
            result.y = (x - p_center.x) * psin + (y - p_center.y) * pcos;
            return (result + p_center);
        }
        /**
         * @brief Rounds down the components of the given vector to the nearest lower integer values.
         *
         * @param p_vector The vector to be rounded down.
         * @return A new vector with each component rounded down.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        static IVector2 floor(const IVector2<TOtherType>& p_vector)
        {
            IVector2<float> result;
            result.x = std::floorf(p_vector.x);
            result.y = std::floorf(p_vector.y);
            return result;
        }

        /**
         * @brief Rounds up the components of the given vector to the nearest higher integer values.
         *
         * @param p_vector The vector to be rounded up.
         * @return A new vector with each component rounded up.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        static IVector2 ceiling(const IVector2<TOtherType>& p_vector)
        {
            IVector2<float> result;
            result.x = std::ceilf(p_vector.x);
            result.y = std::ceilf(p_vector.y);
            return result;
        }

        /**
         * @brief Rounds the components of the given vector to the nearest integer values.
         *
         * @param p_vector The vector to be rounded.
         * @return A new vector with each component rounded to the nearest integer.
         */
        template <typename TOtherType, typename = typename std::enable_if<std::is_arithmetic<TOtherType>::value>::type>
        static IVector2 round(const IVector2<TOtherType>& p_vector)
        {
            IVector2<float> result;
            result.x = std::roundf(p_vector.x);
            result.y = std::roundf(p_vector.y);
            return result;
        }

        /**
         * @brief Determines the component-wise minimum of two vectors.
         *
         * @param p_min The first vector to compare.
         * @param p_max The second vector to compare.
         * @return A new vector where each component is the minimum value of the corresponding components of the input vectors.
         */
        static IVector2 min(const IVector2& p_min, const IVector2& p_max)
        {
            return IVector2(std::min(p_min.x, p_max.x), std::min(p_min.y, p_max.y));
        }

        /**
         * @brief Determines the component-wise maximum of two vectors.
         *
         * @param p_min The first vector to compare.
         * @param p_max The second vector to compare.
         * @return A new vector where each component is the maximum value of the corresponding components of the input vectors.
         */
        static IVector2 max(const IVector2& p_min, const IVector2& p_max)
        {
            return IVector2(std::max(p_min.x, p_max.x), std::max(p_min.y, p_max.y));
        }
        /**
         * @brief Checks if a point is inside the rectangle defined by two corner points.
         *
         * @param p_point The point to check.
         * @param p_cornerA One corner of the rectangle.
         * @param p_cornerB The opposite corner of the rectangle.
         * @return True if the point is inside the rectangle, false otherwise.
         */
        static bool isInsideRectangle(const IVector2& p_point, const IVector2& p_cornerA, const IVector2& p_cornerB)
        {
            IVector2 tmpMin = IVector2::min(p_cornerA, p_cornerB);
            IVector2 tmpMax = IVector2::max(p_cornerA, p_cornerB);
            return !(p_point.x < tmpMin.x || p_point.y < tmpMin.y || p_point.x > tmpMax.x || p_point.y > tmpMax.y);
        }

        /**
         * @brief Performs linear interpolation between two points.
         *
         * @param p_startingPoint The starting point of the interpolation.
         * @param p_endingPoint The ending point of the interpolation.
         * @param t The interpolation factor between 0 and 1.
         * @return The interpolated point.
         */
        static IVector2 lerp(const IVector2& p_startingPoint, const IVector2& p_endingPoint, float t)
        {
            return IVector2(p_startingPoint.x + (p_endingPoint.x - p_startingPoint.x) * t, p_startingPoint.y + (p_endingPoint.y - p_startingPoint.y) * t);
        }

        /**
         * @brief Converts a vector of angles in radians to degrees.
         *
         * @param p_radians The vector containing angles in radians.
         * @return A vector with angles converted to degrees.
         */
        static IVector2 radianToDegree(const IVector2& p_radians)
        {
            return IVector2(spk::radianToDegree(p_radians.x), spk::radianToDegree(p_radians.y));
        }

        /**
         * @brief Converts a vector of angles in degrees to radians.
         *
         * @param p_degrees The vector containing angles in degrees.
         * @return A vector with angles converted to radians.
         */
        static IVector2 degreeToRadian(const IVector2& p_degrees)
        {
            return IVector2(spk::degreeToRadian(p_degrees.x), spk::degreeToRadian(p_degrees.y));
        }

        /**
         * @brief Clamps the current vector components between the specified lower and higher values.
         *
         * @param p_lowerValue The lower bound for the vector components.
         * @param p_higherValue The higher bound for the vector components.
         * @return A new vector with the clamped values.
         */
        IVector2 clamp(const IVector2& p_lowerValue, const IVector2& p_higherValue) const
        {
            return IVector2(std::clamp(x, p_lowerValue.x, p_higherValue.x), std::clamp(y, p_lowerValue.y, p_higherValue.y));
        }
    };

    /**
     * @brief Global operator overload for adding a scalar value to a vector.
     *
     * @tparam TType Type of the vector components.
     * @tparam TValueType Type of the scalar value, must be an arithmetic type.
     * @param p_value The scalar value to add.
     * @param p_point The vector to which the scalar value is added.
     * @return A new vector with the scalar value added to both components of the original vector.
     */
    template <typename TType, typename TValueType, typename = std::enable_if_t<std::is_arithmetic<TValueType>::value>>
    IVector2<TType> operator+(TValueType p_value, const IVector2<TType>& p_point)
    {
        return IVector2<TType>(p_value + p_point.x, p_value + p_point.y);
    }

    /**
     * @brief Global operator overload for subtracting a vector from a scalar value.
     *
     * @tparam TType Type of the vector components.
     * @tparam TValueType Type of the scalar value, must be an arithmetic type.
     * @param p_value The scalar value from which the vector is subtracted.
     * @param p_point The vector to subtract from the scalar value.
     * @return A new vector resulting from the scalar value minus each component of the original vector.
     */
    template <typename TType, typename TValueType, typename = std::enable_if_t<std::is_arithmetic<TValueType>::value>>
    IVector2<TType> operator-(TValueType p_value, const IVector2<TType>& p_point)
    {
        return IVector2<TType>(p_value - p_point.x, p_value - p_point.y);
    }

    /**
     * @brief Global operator overload for multiplying a scalar value with a vector.
     *
     * @tparam TType Type of the vector components.
     * @tparam TValueType Type of the scalar value, must be an arithmetic type.
     * @param p_value The scalar value to multiply.
     * @param p_point The vector to be multiplied by the scalar value.
     * @return A new vector resulting from the scalar value multiplied with each component of the original vector.
     */
    template <typename TType, typename TValueType, typename = std::enable_if_t<std::is_arithmetic<TValueType>::value>>
    IVector2<TType> operator*(TValueType p_value, const IVector2<TType>& p_point)
    {
        return IVector2<TType>(p_value * p_point.x, p_value * p_point.y);
    }

    /**
     * @brief Global operator overload for dividing a scalar value by a vector.
     *
     * @tparam TType Type of the vector components.
     * @tparam TValueType Type of the scalar value, must be an arithmetic type.
     * @param p_value The scalar value to be divided.
     * @param p_point The vector dividing the scalar value.
     * @return A new vector resulting from the scalar value divided by each component of the original vector.
     * @note This operation applies the reciprocal of the vector's components to the scalar value.
     */
    template <typename TType, typename TValueType, typename = std::enable_if_t<std::is_arithmetic<TValueType>::value>>
    IVector2<TType> operator/(TValueType p_value, const IVector2<TType>& p_point)
    {
        return IVector2<TType>(p_value / p_point.x, p_value / p_point.y);
    }

    /**
     * @brief Converts a vector to a string representation.
     *
     * @tparam TType Type of the vector components.
     * @param p_point The vector to convert to a string.
     * @return A string representation of the vector.
     */
    template <typename TType>
    std::string to_string(const IVector2<TType>& p_point)
    {
        return p_point.to_string();
    }

    // Type aliases for commonly used IVector2 types
    using Vector2 = IVector2<float>;            ///< Alias for IVector2 with float components.
    using Vector2Int = IVector2<int>;           ///< Alias for IVector2 with int components.
    using Vector2UInt = IVector2<unsigned int>; ///< Alias for IVector2 with unsigned int components.
}