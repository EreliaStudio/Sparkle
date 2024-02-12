#pragma once

#include <stdexcept>
#include <string>

#include "math/spk_vector2.hpp"
#include "miscellaneous/JSON/spk_JSON_object.hpp"

namespace spk
{
	/**
	 * @interface IVector3
	 * 
	 * @tparam TType The numeric type used to represent the vector's components. It must be an arithmetic type (e.g., int, float, double).
	 * 
	 * @brief Represents a 3D vector with components x, y, and z.
	 *
	 * This class provides a template for creating 3D vectors of various types
	 * (e.g., float, int, unsigned int). It includes common vector operations
	 * such as addition, subtraction, multiplication, division, dot product,
	 * cross product, normalization, and more.
	 */
	template <typename TType>
	struct IVector3
	{
		TType x; //!< X component of the vector.
		TType y; //!< Y component of the vector.
		TType z; //!< Z component of the vector.

		        /**
         * @brief Default constructor. Initializes x, y, and z components to zero.
         */
        IVector3() : x(0), y(0), z(0) {}

        /**
         * @brief Constructs a vector with all components set to the same specified value.
         * @param p_value The value to set for x, y, and z components.
         */
        IVector3(const TType &p_value) : x(p_value), y(p_value), z(p_value) {}

        /**
         * @brief Constructs a 3D vector from a 2D vector and a separate z component value.
         * @param p_other The 2D vector for the x and y components.
         * @param p_value The value for the z component.
         */
        IVector3(const IVector3<TType> &p_other, const TType &p_value) : x(p_other.x), y(p_other.y), z(p_value) {}

        /**
         * @brief Copy constructor. Initializes a new vector with the components of another vector.
         * @param p_other The vector from which to copy the components.
         */
        IVector3(const IVector3<TType> &p_other) : x(p_other.x), y(p_other.y), z(p_other.z) {}

        /**
         * @brief Constructs a vector with separate values for x, y, and z components.
         * @param p_x The value for the x component.
         * @param p_y The value for the y component.
         * @param p_z The value for the z component.
         */
        IVector3(const TType &p_x, const TType &p_y, const TType &p_z) : x(p_x), y(p_y), z(p_z) {}

        /**
         * @brief Conversion operator to allow implicit conversion between vectors of different types.
         * @return A new vector of the target type with copied components.
         */
        operator IVector3<TType>()
        {
            return IVector3<TType>(static_cast<TType>(x), static_cast<TType>(y), static_cast<TType>(z));
        }
        /**
         * @brief Unary negation operator. Negates all components of the vector.
         * @return A new vector where each component is the negation of the original vector's components.
         */
        IVector3 operator-() const
        {
            return IVector3(-x, -y, -z);
        }

        /**
         * @brief Stream insertion operator for printing the vector to an output stream.
         * @param p_os Reference to the output stream.
         * @param p_self The vector to be inserted into the stream.
         * @return Reference to the output stream.
         */
        friend std::ostream &operator<<(std::ostream &p_os, const IVector3 &p_self)
        {
            p_os << p_self.x << " / " << p_self.y << " / " << p_self.z;
            return p_os;
        }

        /**
         * @brief Converts the vector to a string representation.
         * @return A string representing the vector in the format "x / y / z".
         */
        std::string to_string() const
        {
            return std::to_string(x) + " / " + std::to_string(y) + " / " + std::to_string(z);
        }

        /**
         * @brief Adds two vectors component-wise.
         * @param p_other The vector to add.
         * @return A new vector resulting from the component-wise addition.
         */
        IVector3<TType> operator+(const IVector3 &p_other) const
        {
            return IVector3<TType>(x + p_other.x, y + p_other.y, z + p_other.z);
        }

        /**
         * @brief Subtracts two vectors component-wise.
         * @param p_other The vector to subtract.
         * @return A new vector resulting from the component-wise subtraction.
         */
        IVector3<TType> operator-(const IVector3 &p_other) const
        {
            return IVector3<TType>(x - p_other.x, y - p_other.y, z - p_other.z);
        }

        /**
         * @brief Multiplies two vectors component-wise.
         * @param p_other The vector to multiply by.
         * @return A new vector resulting from the component-wise multiplication.
         */
        IVector3<TType> operator*(const IVector3 &p_other) const
        {
            return IVector3<TType>(x * p_other.x, y * p_other.y, z * p_other.z);
        }

        /**
         * @brief Divides two vectors component-wise.
         * @param p_other The vector to divide by.
         * @return A new vector resulting from the component-wise division.
         * @throws std::runtime_error if any component of p_other is zero.
         */
        IVector3<TType> operator/(const IVector3 &p_other) const
        {
            if (p_other.x == 0 || p_other.y == 0 || p_other.z == 0)
            {
                throw std::runtime_error("Attempting to divide by zero in vector division.");
            }
            return IVector3<TType>(x / p_other.x, y / p_other.y, z / p_other.z);
        }
        /**
         * @brief Compound addition operator. Adds another vector to this vector component-wise.
         * @param p_other The vector to add to this one.
         */
        void operator+=(const IVector3 &p_other)
        {
            x += p_other.x;
            y += p_other.y;
            z += p_other.z;
        }

        /**
         * @brief Compound subtraction operator. Subtracts another vector from this vector component-wise.
         * @param p_other The vector to subtract from this one.
         */
        void operator-=(const IVector3 &p_other)
        {
            x -= p_other.x;
            y -= p_other.y;
            z -= p_other.z;
        }

        /**
         * @brief Compound multiplication operator. Multiplies this vector by another vector component-wise.
         * @param p_other The vector to multiply this one by.
         */
        void operator*=(const IVector3 &p_other)
        {
            x *= p_other.x;
            y *= p_other.y;
            z *= p_other.z;
        }

        /**
         * @brief Compound division operator. Divides this vector by another vector component-wise.
         * @param p_other The vector to divide this one by.
         * @throws std::runtime_error if any component of p_other is zero.
         */
        void operator/=(const IVector3 &p_other)
        {
            if (p_other.x == 0 || p_other.y == 0 || p_other.z == 0)
            {
                throw std::runtime_error("Attempting to divide by zero in vector division.");
            }
            x /= p_other.x;
            y /= p_other.y;
            z /= p_other.z;
        }

        /**
         * @brief Equality operator. Checks if all components of this vector are equal to a single scalar value.
         * @param p_other The scalar value to compare against.
         * @return True if all components are equal to p_other, false otherwise.
         */
        bool operator==(const TType &p_other) const
        {
            return x == p_other && y == p_other && z == p_other;
        }

        /**
         * @brief Inequality operator. Checks if any component of this vector is not equal to a single scalar value.
         * @param p_other The scalar value to compare against.
         * @return True if any component is not equal to p_other, false otherwise.
         */
        bool operator!=(const TType &p_other) const
        {
            return x != p_other || y != p_other || z != p_other;
        }
        /**
         * @brief Less than operator. Compares this vector to a scalar.
         * @param p_other The scalar value to compare against.
         * @return True if this vector's components are lexicographically less than the scalar, false otherwise.
         */
        bool operator<(const TType &p_other) const
        {
            return z < p_other ||
                   (z == p_other && y < p_other) ||
                   (z == p_other && y == p_other && x < p_other);
        }

        /**
         * @brief Greater than operator. Compares this vector to a scalar.
         * @param p_other The scalar value to compare against.
         * @return True if this vector's components are lexicographically greater than the scalar, false otherwise.
         */
        bool operator>(const TType &p_other) const
        {
            return z > p_other ||
                   (z == p_other && y > p_other) ||
                   (z == p_other && y == p_other && x > p_other);
        }

        /**
         * @brief Less than or equal to operator. Compares this vector to a scalar.
         * @param p_other The scalar value to compare against.
         * @return True if this vector's components are lexicographically less than or equal to the scalar, false otherwise.
         */
        bool operator<=(const TType &p_other) const
        {
            return *this < p_other || *this == p_other;
        }

        /**
         * @brief Greater than or equal to operator. Compares this vector to a scalar.
         * @param p_other The scalar value to compare against.
         * @return True if this vector's components are lexicographically greater than or equal to the scalar, false otherwise.
         */
        bool operator>=(const TType &p_other) const
        {
            return *this > p_other || *this == p_other;
        }

        /**
         * @brief Equality operator. Compares this vector to another vector.
         * @param p_other The vector to compare against.
         * @return True if all components of the two vectors are equal, false otherwise.
         */
        bool operator==(const IVector3<TType> &p_other) const
        {
            return x == p_other.x && y == p_other.y && z == p_other.z;
        }

        /**
         * @brief Inequality operator. Compares this vector to another vector.
         * @param p_other The vector to compare against.
         * @return True if any component of the two vectors are not equal, false otherwise.
         */
        bool operator!=(const IVector3<TType> &p_other) const
        {
            return !(*this == p_other);
        }

        /**
         * @brief Less than operator. Compares this vector to another vector lexicographically.
         * @param p_other The vector to compare against.
         * @return True if this vector is lexicographically less than the other vector, false otherwise.
         */
        bool operator<(const IVector3<TType> &p_other) const
        {
            return z < p_other.z ||
                   (z == p_other.z && y < p_other.y) ||
                   (z == p_other.z && y == p_other.y && x < p_other.x);
        }

        /**
         * @brief Greater than operator. Compares this vector to another vector lexicographically.
         * @param p_other The vector to compare against.
         * @return True if this vector is lexicographically greater than the other vector, false otherwise.
         */
        bool operator>(const IVector3<TType> &p_other) const
        {
            return z > p_other.z ||
                   (z == p_other.z && y > p_other.y) ||
                   (z == p_other.z && y == p_other.y && x > p_other.x);
        }

        /**
         * @brief Less than or equal to operator. Compares this vector to another vector lexicographically.
         * @param p_other The vector to compare against.
         * @return True if this vector is lexicographically less than or equal to the other vector, false otherwise.
         */
        bool operator<=(const IVector3<TType> &p_other) const
        {
            return *this < p_other || *this == p_other;
        }

        /**
         * @brief Greater than or equal to operator. Compares this vector to another vector lexicographically.
         * @param p_other The vector to compare against.
         * @return True if this vector is lexicographically greater than or equal to the other vector, false otherwise.
         */
        bool operator>=(const IVector3<TType> &p_other) const
        {
            return *this > p_other || *this == p_other;
        }

        /**
         * @brief Calculates the distance between this vector and another vector.
         * @param p_other The other vector.
         * @return The Euclidean distance.
         */
        float distance(const IVector3<TType> &p_other) const
        {
            return std::sqrt(std::pow(p_other.x - x, 2) + std::pow(p_other.y - y, 2) + std::pow(p_other.z - z, 2));
        }

        /**
         * @brief Calculates the norm (length) of this vector.
         * @return The norm of the vector.
         */
        float norm() const
        {
            return std::sqrt(x * x + y * y + z * z);
        }

        /**
         * @brief Normalizes this vector to unit length.
         * @return A new vector with the same direction but a length of 1.
         */
        IVector3<float> normalize() const
        {
            float len = norm();
            if (len == 0) return (IVector3<float>(0, 0, 0));
            return IVector3<float>(x / len, y / len, z / len);
        }

        /**
         * @brief Calculates the cross product of this vector with another vector.
         * @param p_other The other vector.
         * @return The cross product vector.
         */
        IVector3<TType> cross(const IVector3<TType> &p_other) const
        {
            return IVector3<TType>(
                y * p_other.z - z * p_other.y,
                z * p_other.x - x * p_other.z,
                x * p_other.y - y * p_other.x);
        }

        /**
         * @brief Rotates this vector by given rotation values (in degrees) around each axis.
         * @param p_rotationValues The rotation values for x, y, and z axes in degrees.
         * @return The rotated vector.
         */
        IVector3<TType> rotate(const IVector3<TType> &p_rotationValues) const
        {
            // Conversion of rotation values from degrees to radians and calculation of sine and cosine for each
            TType cos_x = std::cos(spk::degreeToRadian(p_rotationValues.x));
            TType sin_x = std::sin(spk::degreeToRadian(p_rotationValues.x));
            TType cos_y = std::cos(spk::degreeToRadian(p_rotationValues.y));
            TType sin_y = std::sin(spk::degreeToRadian(p_rotationValues.y));
            TType cos_z = std::cos(spk::degreeToRadian(p_rotationValues.z));
            TType sin_z = std::sin(spk::degreeToRadian(p_rotationValues.z));

            // Application of rotation matrix
            IVector3<TType> result;
            result.x = cos_y * cos_z * x + (sin_x * sin_y * cos_z - cos_x * sin_z) * y + (cos_x * sin_y * cos_z + sin_x * sin_z) * z;
            result.y = cos_y * sin_z * x + (sin_x * sin_y * sin_z + cos_x * cos_z) * y + (cos_x * sin_y * sin_z - sin_x * cos_z) * z;
            result.z = -sin_y * x + sin_x * cos_y * y + cos_x * cos_y * z;

            return result;
        }

        /**
         * @brief Calculates the dot product of this vector with another vector.
         * @param p_other The other vector.
         * @return The dot product.
         */
        TType dot(const IVector3<TType> &p_other) const
        {
            return x * p_other.x + y * p_other.y + z * p_other.z;
        }

        /**
         * @brief Projects this vector onto a plane defined by a normal and a point on the plane.
         * @param p_planeNormal The normal of the plane.
         * @param p_planePoint A point on the plane.
         * @return The projection of this vector onto the plane.
         */
        IVector3 projectOnPlane(const IVector3 &p_planeNormal, const IVector3 &p_planePoint) const
        {
            IVector3 pointToPlane = *this - p_planePoint;
            float dist = pointToPlane.dot(p_planeNormal);
            return *this - p_planeNormal * dist;
        }

        /**
         * @brief Removes the Y component, effectively projecting the vector onto the XZ plane.
         * @return A 2D vector containing the X and Z components of this vector.
         */
        IVector3<TType> removeY()
        {
            return IVector3<TType>(x, z);
        }

        /**
         * @brief Inverts the direction of the vector.
         * @return A vector where each component is the negation of the original vector's corresponding component.
         */
        IVector3 inverse() const
        {
            return IVector3(-x, -y, -z);
        }

        IVector2<TType> xy()
        {
            return (IVector2<TType>(x, y));
        }

        IVector2<TType> xz()
        {
            return (IVector2<TType>(x, z));
        }

        IVector2<TType> yz()
        {
            return (IVector2<TType>(y, z));
        }

        /**
         * @brief Converts a vector from radians to degrees.
         * @param radians The vector in radians to be converted.
         * @return A vector where each component has been converted from radians to degrees.
         */
        static IVector3 radianToDegree(const IVector3& radians)
        {
            return IVector3(
                    spk::radianToDegree(radians.x),
                    spk::radianToDegree(radians.y),
                    spk::radianToDegree(radians.z)
                );
        }

        /**
         * @brief Converts a vector from degrees to radians.
         * @param degrees The vector in degrees to be converted.
         * @return A vector where each component has been converted from degrees to radians.
         */
        static IVector3 degreeToRadian(const IVector3& degrees)
        {
            return IVector3(
                    spk::degreeToRadian(degrees.x),
                    spk::degreeToRadian(degrees.y),
                    spk::degreeToRadian(degrees.z)
                );
        }

        /**
         * @brief Clamps the vector components to be within the specified range.
         * @param p_lowerValue The minimum allowable value for each component.
         * @param p_higherValue The maximum allowable value for each component.
         * @return A vector where each component has been clamped to the specified range.
         */
        IVector3 clamp(const IVector3& p_lowerValue, const IVector3& p_higherValue)
        {
            return IVector3(
                std::clamp(x, p_lowerValue.x, p_higherValue.x), 
                std::clamp(y, p_lowerValue.y, p_higherValue.y), 
                std::clamp(z, p_lowerValue.z, p_higherValue.z)
            );
        }

        /**
         * @brief Rounds down the components of the given vector to the nearest lower integer values.
         * 
         * @param p_vector The vector to be rounded down.
         * @return A new vector with each component rounded down.
         */
        static IVector3 floor(const IVector3 &p_vector)
        {
            IVector3 result;
            result.x = std::floor(p_vector.x);
            result.y = std::floor(p_vector.y);
            result.z = std::floor(p_vector.z);
            return result;
        }

        /**
         * @brief Rounds up the components of the given vector to the nearest higher integer values.
         * 
         * @param p_vector The vector to be rounded up.
         * @return A new vector with each component rounded up.
         */
        static IVector3 ceiling(const IVector3 &p_vector)
        {
            IVector3 result;
            result.x = std::ceil(p_vector.x);
            result.y = std::ceil(p_vector.y);
            result.z = std::ceil(p_vector.z);
            return result;
        }

        /**
         * @brief Rounds the components of the given vector to the nearest integer values.
         * 
         * @param p_vector The vector to be rounded.
         * @return A new vector with each component rounded to the nearest integer.
         */
        static IVector3 round(const IVector3 &p_vector)
        {
            IVector3 result;
            result.x = std::round(p_vector.x);
            result.y = std::round(p_vector.y);
            result.z = std::round(p_vector.z);
            return result;
        }

        /**
         * @brief Determines the component-wise minimum of two vectors.
         * 
         * @param p_min The first vector to compare.
         * @param p_max The second vector to compare.
         * @return A new vector where each component is the minimum value of the corresponding components of the input vectors.
         */
        static IVector3 min(const IVector3 &p_min, const IVector3 &p_max)
        {
            return IVector3(std::min(p_min.x, p_max.x), std::min(p_min.y, p_max.y), std::min(p_min.z, p_max.z));
        }

        /**
         * @brief Determines the component-wise maximum of two vectors.
         * 
         * @param p_min The first vector to compare.
         * @param p_max The second vector to compare.
         * @return A new vector where each component is the maximum value of the corresponding components of the input vectors.
         */
        static IVector3 max(const IVector3 &p_min, const IVector3 &p_max)
        {
            return IVector3(std::max(p_min.x, p_max.x), std::max(p_min.y, p_max.y), std::max(p_min.z, p_max.z));
        }
        /**
         * @brief Checks if a point is inside the rectangle defined by two corner points.
         * 
         * @param p_point The point to check.
         * @param p_cornerA One corner of the rectangle.
         * @param p_cornerB The opposite corner of the rectangle.
         * @return True if the point is inside the rectangle, false otherwise.
         */
        static bool isInsideRectangle(const IVector3 &p_point, const IVector3 &p_cornerA, const IVector3 &p_cornerB)
        {
            IVector3 tmpMin = IVector3::min(p_cornerA, p_cornerB);
            IVector3 tmpMax = IVector3::max(p_cornerA, p_cornerB);
            return (!(
                    p_point.x < tmpMin.x || p_point.y < tmpMin.y || p_point.z < tmpMin.z ||
                    p_point.x > tmpMax.x || p_point.y > tmpMax.y || p_point.z > tmpMax.z
                ));
        }

        /**
         * @brief Performs linear interpolation between two points.
         * 
         * @param p_startingPoint The starting point of the interpolation.
         * @param p_endingPoint The ending point of the interpolation.
         * @param t The interpolation factor between 0 and 1.
         * @return The interpolated point.
         */
        static IVector3 lerp(const IVector3 &p_startingPoint, const IVector3 &p_endingPoint, float t)
        {
            return IVector3(
                    p_startingPoint.x + (p_endingPoint.x - p_startingPoint.x) * t,
                    p_startingPoint.y + (p_endingPoint.y - p_startingPoint.y) * t,
                    p_startingPoint.z + (p_endingPoint.z - p_startingPoint.z) * t
                );
        }
	};
        /**
         * @brief Adds a scalar value to each component of a 3D vector.
         * @param p_value The scalar value to add.
         * @param p_point The 3D vector to which the scalar value will be added.
         * @return A new vector resulting from the addition of the scalar value to each component of the original vector.
         */
        template <typename TType, typename TValueType, typename = std::enable_if_t<std::is_arithmetic<TValueType>::value>>
        IVector3<TType> operator+(TValueType p_value, const IVector3<TType> &p_point)
        {
            return IVector3<TType>(p_value) + p_point;
        };

        /**
         * @brief Subtracts a scalar value from each component of a 3D vector.
         * @param p_value The scalar value to subtract.
         * @param p_point The 3D vector from which the scalar value will be subtracted.
         * @return A new vector resulting from the subtraction of the scalar value from each component of the original vector.
         */
        template <typename TType, typename TValueType, typename = std::enable_if_t<std::is_arithmetic<TValueType>::value>>
        IVector3<TType> operator-(TValueType p_value, const IVector3<TType> &p_point)
        {
            return IVector3<TType>(p_value) - p_point;
        };

        /**
         * @brief Multiplies each component of a 3D vector by a scalar value.
         * @param p_value The scalar value to multiply.
         * @param p_point The 3D vector whose components will be multiplied by the scalar value.
         * @return A new vector resulting from the multiplication of each component of the original vector by the scalar value.
         */
        template <typename TType, typename TValueType, typename = std::enable_if_t<std::is_arithmetic<TValueType>::value>>
        IVector3<TType> operator*(TValueType p_value, const IVector3<TType> &p_point)
        {
            return IVector3<TType>(p_value) * p_point;
        };

        /**
         * @brief Divides each component of a 3D vector by a scalar value.
         * @param p_value The scalar value to divide by.
         * @param p_point The 3D vector whose components will be divided by the scalar value.
         * @return A new vector resulting from the division of each component of the original vector by the scalar value.
         */
        template <typename TType, typename TValueType, typename = std::enable_if_t<std::is_arithmetic<TValueType>::value>>
        IVector3<TType> operator/(TValueType p_value, const IVector3<TType> &p_point)
        {
            return IVector3<TType>(p_value) / p_point;
        };

        /**
         * @brief Converts a 3D vector to a string representation.
         * @param p_point The 3D vector to convert.
         * @return A string representation of the 3D vector.
         */
        template <typename TType>
        std::string to_string(const IVector3<TType> &p_point)
        {
            return p_point.to_string();
        }

	// Type aliases for commonly used IVector3 types
	using Vector3 = IVector3<float>; ///< Alias for IVector3 with float components.
	using Vector3Int = IVector3<int>; ///< Alias for IVector3 with int components.
	using Vector3UInt = IVector3<unsigned int>; ///< Alias for IVector3 with unsigned int components.

};
