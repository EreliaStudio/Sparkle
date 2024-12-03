#pragma once

#include <cmath>
#include <iostream>
#include <type_traits>
#include <unordered_map>
#include <sstream>
#include <algorithm>

#include "structure/math/spk_math.hpp"

namespace spk
{
	/**
	 * @brief A template class for a 2D vector of arbitrary numeric type.
	 *
	 * This class provides various operations for 2D vectors, including arithmetic,
	 * comparisons, and utility functions.
	 *
	 * @tparam TType The numeric type of the vector components (e.g., float, int).
	 *
	 * Example usage:
	 * @code
	 * spk::Vector2 vec1(1.0f, 2.0f);
	 * spk::Vector2 vec2 = vec1 + spk::Vector2(3.0f, 4.0f);
	 * std::cout << "Vector sum: " << vec2.to_string() << std::endl;
	 * @endcode
	 */
	template <typename TType>
	struct IVector2
	{
		static_assert(std::is_arithmetic<TType>::value, "TType must be an arithmetic type.");

		TType x, y;

		/**
		 * @brief Default constructor. Initializes the vector to (0, 0).
		 */
		IVector2() : x(0), y(0) {}

		/**
		 * @brief Initializes both components of the vector to the specified scalar value.
		 *
		 * @tparam UType The type of the scalar (must be arithmetic).
		 * @param p_scalar The scalar value.
		 */
		template <typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
		IVector2(UType p_scalar) : x(static_cast<TType>(p_scalar)), y(static_cast<TType>(p_scalar)) {}

		/**
		 * @brief Initializes the vector with specified x and y values.
		 *
		 * @tparam UType The type of the x-value (must be arithmetic).
		 * @tparam VType The type of the y-value (must be arithmetic).
		 * @param p_xVal The x-value.
		 * @param p_yVal The y-value.
		 */
		template <typename UType, typename VType,
				  typename = std::enable_if_t<std::is_arithmetic<UType>::value && std::is_arithmetic<VType>::value>>
		IVector2(UType p_xVal, VType p_yVal) : x(static_cast<TType>(p_xVal)), y(static_cast<TType>(p_yVal)) {}

		/**
		 * @brief Copy constructor to convert between different vector types.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The vector to copy from.
		 */
		template <typename UType>
		IVector2(const IVector2<UType> &p_other) : x(static_cast<TType>(p_other.x)), y(static_cast<TType>(p_other.y)) {}

		/**
		 * @brief Converts this vector to a vector of another type.
		 *
		 * @tparam UType The type to convert to.
		 * @return A vector of the specified type.
		 */
		template <typename UType>
		explicit operator IVector2<UType>() const
		{
			return IVector2<UType>(static_cast<UType>(x), static_cast<UType>(y));
		}

		/**
		 * @brief Copy assignment operator to convert between different vector types.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The vector to copy from.
		 */
		template <typename UType>
		IVector2 &operator=(const IVector2<UType> &p_other)
		{
			x = static_cast<TType>(p_other.x);
			y = static_cast<TType>(p_other.y);

			return (*this);
		}

		/**
		 * @brief Inserts the vector into a wide output stream.
		 *
		 * @param p_os The output stream.
		 * @param p_vec The vector to insert.
		 * @return The output stream.
		 */
		friend std::wostream &operator<<(std::wostream &p_os, const IVector2 &p_vec)
		{
			p_os << L"(" << p_vec.x << L", " << p_vec.y << L")";
			return p_os;
		}

		/**
		 * @brief Inserts the vector into a standard output stream.
		 *
		 * @param p_os The output stream.
		 * @param p_vec The vector to insert.
		 * @return The output stream.
		 */
		friend std::ostream &operator<<(std::ostream &p_os, const IVector2 &p_vec)
		{
			p_os << "(" << p_vec.x << ", " << p_vec.y << ")";
			return p_os;
		}

		/**
		 * @brief Converts the vector to a wide string representation.
		 *
		 * @return A wide string containing the vector components in the format "(x, y)".
		 */
		std::wstring to_wstring() const
		{
			std::wstringstream wss;
			wss << *this;
			return wss.str();
		}

		/**
		 * @brief Converts the vector to a string representation.
		 *
		 * @return A string containing the vector components in the format "(x, y)".
		 */
		std::string to_string() const
		{
			std::stringstream ss;
			ss << *this;
			return ss.str();
		}

		/**
		 * @brief Compares this vector with another vector for equality.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The vector to compare against.
		 * @return True if the vectors are equal, considering a small epsilon for floating-point types, otherwise false.
		 */
		template <typename UType>
		bool operator==(const IVector2<UType> &p_other) const
		{
			if constexpr (std::is_floating_point<TType>::value)
			{
				constexpr TType epsilon = static_cast<TType>(1e-5);
				return std::fabs(x - static_cast<TType>(p_other.x)) < epsilon &&
					   std::fabs(y - static_cast<TType>(p_other.y)) < epsilon;
			}
			else
			{
				return x == static_cast<TType>(p_other.x) && y == static_cast<TType>(p_other.y);
			}
		}

		/**
		 * @brief Compares this vector with a scalar for equality.
		 *
		 * @tparam UType The type of the scalar.
		 * @param p_scalar The scalar to compare against.
		 * @return True if both components of the vector are equal to the scalar, considering a small epsilon for floating-point types, otherwise false.
		 */
		template <typename UType>
		bool operator==(UType p_scalar) const
		{
			if constexpr (std::is_floating_point<TType>::value)
			{
				constexpr TType epsilon = static_cast<TType>(1e-5);
				return std::fabs(x - static_cast<TType>(p_scalar)) < epsilon &&
					   std::fabs(y - static_cast<TType>(p_scalar)) < epsilon;
			}
			else
			{
				return x == static_cast<TType>(p_scalar) && y == static_cast<TType>(p_scalar);
			}
		}

		/**
		 * @brief Compares this vector with another vector for inequality.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The vector to compare against.
		 * @return True if the vectors are not equal, otherwise false.
		 */
		template <typename UType>
		bool operator!=(const IVector2<UType> &p_other) const
		{
			return !(*this == p_other);
		}

		/**
		 * @brief Compares this vector with another vector for less-than order.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The vector to compare against.
		 * @return True if this vector is less than the other vector, otherwise false.
		 */
		template <typename UType>
		bool operator<(const IVector2<UType> &p_other) const
		{
			return x < p_other.x || (x == p_other.x && y < p_other.y);
		}

		/**
		 * @brief Compares this vector with another vector for greater-than order.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The vector to compare against.
		 * @return True if this vector is greater than the other vector, otherwise false.
		 */
		template <typename UType>
		bool operator>(const IVector2<UType> &p_other) const
		{
			return x > p_other.x || (x == p_other.x && y > p_other.y);
		}

		/**
		 * @brief Compares this vector with another vector for less-than-or-equal order.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The vector to compare against.
		 * @return True if this vector is less than or equal to the other vector, otherwise false.
		 */
		template <typename UType>
		bool operator<=(const IVector2<UType> &p_other) const
		{
			return !(*this > p_other);
		}

		/**
		 * @brief Compares this vector with another vector for greater-than-or-equal order.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The vector to compare against.
		 * @return True if this vector is greater than or equal to the other vector, otherwise false.
		 */
		template <typename UType>
		bool operator>=(const IVector2<UType> &p_other) const
		{
			return !(*this < p_other);
		}

		/**
		 * @brief Compares this vector with a scalar for inequality.
		 *
		 * @tparam UType The type of the scalar.
		 * @param p_scalar The scalar to compare against.
		 * @return True if either component of the vector is not equal to the scalar, otherwise false.
		 */
		template <typename UType>
		bool operator!=(UType p_scalar) const
		{
			return !(*this == p_scalar);
		}

		/**
		 * @brief Compares this vector with a scalar for less-than order.
		 *
		 * @tparam UType The type of the scalar.
		 * @param p_scalar The scalar to compare against.
		 * @return True if both components of the vector are less than the scalar, otherwise false.
		 */
		template <typename UType>
		bool operator<(UType p_scalar) const
		{
			return x < p_scalar && y < p_scalar;
		}

		/**
		 * @brief Compares this vector with a scalar for greater-than order.
		 *
		 * @tparam UType The type of the scalar.
		 * @param p_scalar The scalar to compare against.
		 * @return True if both components of the vector are greater than the scalar, otherwise false.
		 */
		template <typename UType>
		bool operator>(UType p_scalar) const
		{
			return x > p_scalar && y > p_scalar;
		}

		/**
		 * @brief Compares this vector with a scalar for less-than-or-equal order.
		 *
		 * @tparam UType The type of the scalar.
		 * @param p_scalar The scalar to compare against.
		 * @return True if both components of the vector are less than or equal to the scalar, otherwise false.
		 */
		template <typename UType>
		bool operator<=(UType p_scalar) const
		{
			return x <= p_scalar && y <= p_scalar;
		}

		/**
		 * @brief Compares this vector with a scalar for greater-than-or-equal order.
		 *
		 * @tparam UType The type of the scalar.
		 * @param p_scalar The scalar to compare against.
		 * @return True if both components of the vector are greater than or equal to the scalar, otherwise false.
		 */
		template <typename UType>
		bool operator>=(UType p_scalar) const
		{
			return x >= p_scalar && y >= p_scalar;
		}

		/**
		 * @brief Negates the vector, reversing the sign of each component.
		 *
		 * @return A new vector where each component is the negation of the corresponding component in this vector.
		 */
		IVector2<TType> operator-() const
		{
			return IVector2<TType>(-x, -y);
		}

		/**
		 * @brief Adds two vectors.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The vector to add.
		 * @return The resulting vector.
		 */
		template <typename UType>
		IVector2<TType> operator+(const IVector2<UType> &p_other) const
		{
			return IVector2<TType>(x + p_other.x, y + p_other.y);
		}

		/**
		 * @brief Subtracts another vector from this vector.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The vector to subtract.
		 * @return The resulting vector.
		 */
		template <typename UType>
		IVector2<TType> operator-(const IVector2<UType> &p_other) const
		{
			return IVector2<TType>(x - p_other.x, y - p_other.y);
		}

		/**
		 * @brief Multiplies this vector with another vector element-wise.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The vector to multiply with.
		 * @return A new vector where each component is the product of the corresponding components in the two vectors.
		 */
		template <typename UType>
		IVector2<TType> operator*(const IVector2<UType> &p_other) const
		{
			return IVector2<TType>(x * p_other.x, y * p_other.y);
		}

		/**
		 * @brief Divides this vector by another vector element-wise.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The vector to divide by.
		 * @return A new vector where each component is the result of dividing the corresponding components in this vector by the other vector.
		 * @throw std::runtime_error If any component of the other vector is zero.
		 */
		template <typename UType>
		IVector2<TType> operator/(const IVector2<UType> &p_other) const
		{
			if (p_other.x == 0 || p_other.y == 0)
			{
				throw std::runtime_error("Division by zero");
			}
			return IVector2<TType>(x / p_other.x, y / p_other.y);
		}

		/**
		 * @brief Adds a scalar value to each component of the vector.
		 *
		 * @tparam UType The type of the scalar.
		 * @param p_scalar The scalar to add.
		 * @return A new vector with the scalar added to each component.
		 */
		// Arithmetic operators with a p_scalar
		template <typename UType>
		IVector2<TType> operator+(UType p_scalar) const
		{
			return IVector2<TType>(x + p_scalar, y + p_scalar);
		}

		/**
		 * @brief Subtracts a scalar value from each component of the vector.
		 *
		 * @tparam UType The type of the scalar.
		 * @param p_scalar The scalar to subtract.
		 * @return A new vector with the scalar subtracted from each component.
		 */
		template <typename UType>
		IVector2<TType> operator-(UType p_scalar) const
		{
			return IVector2<TType>(x - p_scalar, y - p_scalar);
		}

		/**
		 * @brief Multiplies each component of the vector by a scalar value.
		 *
		 * @tparam UType The type of the scalar.
		 * @param p_scalar The scalar to multiply with.
		 * @return A new vector with each component multiplied by the scalar.
		 */
		template <typename UType>
		IVector2<TType> operator*(UType p_scalar) const
		{
			return IVector2<TType>(x * p_scalar, y * p_scalar);
		}

		/**
		 * @brief Divides each component of the vector by a scalar value.
		 *
		 * @tparam UType The type of the scalar.
		 * @param p_scalar The scalar to divide by.
		 * @return A new vector with each component divided by the scalar.
		 * @throw std::runtime_error If the scalar is zero.
		 */
		template <typename UType>
		IVector2<TType> operator/(UType p_scalar) const
		{
			if (p_scalar == 0)
			{
				throw std::runtime_error("Division by zero");
			}
			return IVector2<TType>(x / p_scalar, y / p_scalar);
		}

		/**
		 * @brief Adds another vector to this vector element-wise.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The vector to add.
		 * @return A reference to this vector after modification.
		 */
		template <typename UType>
		IVector2<TType> &operator+=(const IVector2<UType> &p_other)
		{
			x += p_other.x;
			y += p_other.y;
			return *this;
		}

		/**
		 * @brief Subtracts another vector from this vector element-wise.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The vector to subtract.
		 * @return A reference to this vector after modification.
		 */
		template <typename UType>
		IVector2<TType> &operator-=(const IVector2<UType> &p_other)
		{
			x -= p_other.x;
			y -= p_other.y;
			return *this;
		}

		/**
		 * @brief Multiplies this vector with another vector element-wise.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The vector to multiply with.
		 * @return A reference to this vector after modification.
		 */
		template <typename UType>
		IVector2<TType> &operator*=(const IVector2<UType> &p_other)
		{
			x *= p_other.x;
			y *= p_other.y;
			return *this;
		}

		/**
		 * @brief Divides this vector by another vector element-wise.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The vector to divide by.
		 * @return A reference to this vector after modification.
		 * @throw std::runtime_error If any component of the other vector is zero.
		 */
		template <typename UType>
		IVector2<TType> &operator/=(const IVector2<UType> &p_other)
		{
			if (p_other.x == 0 || p_other.y == 0)
			{
				throw std::runtime_error("Division by zero");
			}
			x /= p_other.x;
			y /= p_other.y;
			return *this;
		}

		/**
		 * @brief Adds a scalar value to each component of the vector.
		 *
		 * @tparam UType The type of the scalar.
		 * @param p_scalar The scalar to add.
		 * @return A reference to this vector after modification.
		 */
		template <typename UType>
		IVector2<TType> &operator+=(UType p_scalar)
		{
			x += static_cast<TType>(p_scalar);
			y += static_cast<TType>(p_scalar);
			return *this;
		}

		/**
		 * @brief Subtracts a scalar value from each component of the vector.
		 *
		 * @tparam UType The type of the scalar.
		 * @param p_scalar The scalar to subtract.
		 * @return A reference to this vector after modification.
		 */
		template <typename UType>
		IVector2<TType> &operator-=(UType p_scalar)
		{
			x -= static_cast<TType>(p_scalar);
			y -= static_cast<TType>(p_scalar);
			return *this;
		}

		/**
		 * @brief Multiplies each component of the vector by a scalar value.
		 *
		 * @tparam UType The type of the scalar.
		 * @param p_scalar The scalar to multiply with.
		 * @return A reference to this vector after modification.
		 */
		template <typename UType>
		IVector2<TType> &operator*=(UType p_scalar)
		{
			x *= static_cast<TType>(p_scalar);
			y *= static_cast<TType>(p_scalar);
			return *this;
		}

		/**
		 * @brief Divides each component of the vector by a scalar value.
		 *
		 * @tparam UType The type of the scalar.
		 * @param p_scalar The scalar to divide by.
		 * @return A reference to this vector after modification.
		 * @throw std::runtime_error If the scalar is zero.
		 */
		template <typename UType>
		IVector2<TType> &operator/=(UType p_scalar)
		{
			if (p_scalar == 0)
			{
				throw std::runtime_error("Division by zero");
			}

			x /= static_cast<TType>(p_scalar);
			y /= static_cast<TType>(p_scalar);
			return *this;
		}

		/**
		 * @brief Calculates the distance between this vector and another vector.
		 *
		 * @tparam TOtherType The type of the other vector's components.
		 * @param p_other The other vector.
		 * @return The distance between the two vectors.
		 */
		template <typename TOtherType>
		float distance(const IVector2<TOtherType> &p_other) const
		{
			return static_cast<float>(std::sqrt(std::pow(p_other.x - x, 2) + std::pow(p_other.y - y, 2)));
		}

		/**
		 * @brief Computes the norm (length) of the vector.
		 *
		 * @return The norm of the vector.
		 */
		float norm() const
		{
			return static_cast<float>(std::sqrt(x * x + y * y));
		}

		/**
		 * @brief Normalizes the vector to have a length of 1.
		 *
		 * @return A new normalized vector.
		 * @throw std::runtime_error If the vector has a length of 0.
		 */
		IVector2<float> normalize() const
		{
			float len = norm();
			if (len == 0)
				throw std::runtime_error("Can't calculated a norm for a vector of len 0");
			return IVector2<float>(x / len, y / len);
		}

		/**
		 * @brief Rotates the vector by a given angle.
		 *
		 * @param p_angle The angle of rotation in degrees.
		 * @return A new rotated vector.
		 */
		IVector2<TType> rotate(const TType &p_angle) const
		{
			TType radian = spk::degreeToRadian(static_cast<float>(p_angle));
			TType cos_a = std::cos(radian);
			TType sin_a = std::sin(radian);

			return IVector2<TType>(
				x * cos_a - y * sin_a,
				x * sin_a + y * cos_a);
		}

		/**
		 * @brief Computes the perpendicular vector (cross product in 2D).
		 *
		 * @return A new vector that is perpendicular to this vector.
		 */
		IVector2<TType> cross() const
		{
			return IVector2<TType>(-y, x);
		}

		/**
		 * @brief Computes the dot product with another vector.
		 *
		 * @param p_other The other vector.
		 * @return The dot product of the two vectors.
		 */
		TType dot(const IVector2<TType> &p_other) const
		{
			return x * p_other.x + y * p_other.y;
		}

		/**
		 * @brief Computes the inverse (negation) of the vector.
		 *
		 * @return A new vector where each component is negated.
		 */
		IVector2<TType> inverse() const
		{
			return IVector2<TType>(-x, -y);
		}

		/**
		 * @brief Converts a vector from radians to degrees.
		 *
		 * @param p_radians The vector in radians.
		 * @return A new vector in degrees.
		 */
		static IVector2 radianToDegree(const IVector2 &p_radians)
		{
			return IVector2(
				spk::radianToDegree(p_radians.x),
				spk::radianToDegree(p_radians.y));
		}

		/**
		 * @brief Converts a vector from degrees to radians.
		 *
		 * @param p_degrees The vector in degrees.
		 * @return A new vector in radians.
		 */
		static IVector2 degreeToRadian(const IVector2 &p_degrees)
		{
			return IVector2(
				spk::degreeToRadian(p_degrees.x),
				spk::degreeToRadian(p_degrees.y));
		}

		/**
		 * @brief Clamps the vector's components between two other vectors.
		 *
		 * @param p_lowerValue The lower bound vector.
		 * @param p_higherValue The upper bound vector.
		 * @return A new clamped vector.
		 */
		IVector2 clamp(const IVector2 &p_lowerValue, const IVector2 &p_higherValue)
		{
			return IVector2(
				std::clamp(x, p_lowerValue.x, p_higherValue.x),
				std::clamp(y, p_lowerValue.y, p_higherValue.y));
		}

		/**
		 * @brief Computes the floor of each component of the vector.
		 *
		 * @tparam TOtherType The type of the input vector's components.
		 * @param p_vector The input vector.
		 * @return A new vector with the floored components.
		 */
		template <typename TOtherType>
		static IVector2 floor(const IVector2<TOtherType> &p_vector)
		{
			IVector2 result;
			result.x = std::floor(p_vector.x);
			result.y = std::floor(p_vector.y);
			return result;
		}

		/**
		 * @brief Computes the ceiling of each component of the vector.
		 *
		 * @tparam TOtherType The type of the input vector's components.
		 * @param p_vector The input vector.
		 * @return A new vector with the ceiling components.
		 */
		template <typename TOtherType>
		static IVector2 ceiling(const IVector2<TOtherType> &p_vector)
		{
			IVector2 result;
			result.x = std::ceil(p_vector.x);
			result.y = std::ceil(p_vector.y);
			return result;
		}

		/**
		 * @brief Rounds each component of the vector to the nearest integer.
		 *
		 * @tparam TOtherType The type of the input vector's components.
		 * @param p_vector The input vector.
		 * @return A new vector with the rounded components.
		 */
		template <typename TOtherType>
		static IVector2 round(const IVector2<TOtherType> &p_vector)
		{
			IVector2 result;
			result.x = std::round(p_vector.x);
			result.y = std::round(p_vector.y);
			return result;
		}

		/**
		 * @brief Computes the element-wise minimum of two vectors.
		 *
		 * @param p_min The first vector.
		 * @param p_max The second vector.
		 * @return A new vector containing the element-wise minimum values.
		 */
		static IVector2 min(const IVector2 &p_min, const IVector2 &p_max)
		{
			return IVector2(std::min(p_min.x, p_max.x), std::min(p_min.y, p_max.y));
		}

		/**
		 * @brief Computes the element-wise maximum of two vectors.
		 *
		 * @param p_min The first vector.
		 * @param p_max The second vector.
		 * @return A new vector containing the element-wise maximum values.
		 */
		static IVector2 max(const IVector2 &p_min, const IVector2 &p_max)
		{
			return IVector2(std::max(p_min.x, p_max.x), std::max(p_min.y, p_max.y));
		}

		/**
		 * @brief Performs linear interpolation between two vectors.
		 *
		 * @param p_startingPoint The starting vector.
		 * @param p_endingPoint The ending vector.
		 * @param t The interpolation factor (0.0 to 1.0).
		 * @return A new interpolated vector.
		 */
		static IVector2 lerp(const IVector2 &p_startingPoint, const IVector2 &p_endingPoint, float t)
		{
			return IVector2(
				p_startingPoint.x + (p_endingPoint.x - p_startingPoint.x) * t,
				p_startingPoint.y + (p_endingPoint.y - p_startingPoint.y) * t);
		}
	};

	/**
	 * @brief Alias for a 2D vector of `float`.
	 */
	using Vector2 = IVector2<float_t>;

	/**
	 * @brief Alias for a 2D vector of `int32_t`.
	 */
	using Vector2Int = IVector2<int32_t>;

	/**
	 * @brief Alias for a 2D vector of `uint32_t`.
	 */
	using Vector2UInt = IVector2<uint32_t>;
}

/**
 * @brief Adds a scalar value to each component of a vector.
 *
 * @tparam TType The type of the vector components.
 * @tparam UType The type of the scalar (must be arithmetic).
 * @param p_scalar The scalar to add.
 * @param p_vec The vector to which the scalar is added.
 * @return A new vector with the scalar added to each component.
 */
template <typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector2<TType> operator+(UType p_scalar, const spk::IVector2<TType> &p_vec)
{
	return spk::IVector2<TType>(p_scalar + p_vec.x, p_scalar + p_vec.y);
}

/**
 * @brief Subtracts each component of a vector from a scalar value.
 *
 * @tparam TType The type of the vector components.
 * @tparam UType The type of the scalar (must be arithmetic).
 * @param p_scalar The scalar from which the vector components are subtracted.
 * @param p_vec The vector to subtract.
 * @return A new vector where each component is the result of subtracting the vector component from the scalar.
 */
template <typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector2<TType> operator-(UType p_scalar, const spk::IVector2<TType> &p_vec)
{
	return spk::IVector2<TType>(p_scalar - p_vec.x, p_scalar - p_vec.y);
}

/**
 * @brief Multiplies each component of a vector by a scalar value.
 *
 * @tparam TType The type of the vector components.
 * @tparam UType The type of the scalar (must be arithmetic).
 * @param p_scalar The scalar to multiply with.
 * @param p_vec The vector to multiply.
 * @return A new vector with each component multiplied by the scalar.
 */
template <typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector2<TType> operator*(UType p_scalar, const spk::IVector2<TType> &p_vec)
{
	return spk::IVector2<TType>(p_scalar * p_vec.x, p_scalar * p_vec.y);
}

/**
 * @brief Divides a scalar value by each component of a vector.
 *
 * @tparam TType The type of the vector components.
 * @tparam UType The type of the scalar (must be arithmetic).
 * @param p_scalar The scalar to divide.
 * @param p_vec The vector by which the scalar is divided.
 * @return A new vector where each component is the result of dividing the scalar by the corresponding vector component.
 * @throw std::runtime_error If any component of the vector is zero.
 */
template <typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector2<TType> operator/(UType p_scalar, const spk::IVector2<TType> &p_vec)
{
	if (p_vec.x == 0 || p_vec.y == 0)
	{
		throw std::runtime_error("Division by zero");
	}
	return spk::IVector2<TType>(p_scalar / p_vec.x, p_scalar / p_vec.y);
}

/**
 * @brief Specialization of `std::hash` for `spk::IVector2`.
 *
 * Allows `spk::IVector2` to be used as a key in `std::unordered_map` and other hash-based containers.
 *
 * @tparam TType The type of the vector components.
 */
namespace std
{
	template <typename TType>
	struct hash<spk::IVector2<TType>>
	{
		/**
		 * @brief Computes a hash value for the given vector.
		 *
		 * @param p_vec The vector to hash.
		 * @return A hash value for the vector.
		 */
		size_t operator()(const spk::IVector2<TType> &p_vec) const
		{
			return hash<TType>()(p_vec.x) ^ (hash<TType>()(p_vec.y) << 1);
		}
	};
}