#pragma once
	
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>

#include <string>
#include <iostream>
#include <algorithm>

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
	template <typename TType>
	struct IVector2
	{
		TType x;
		TType y;

		/**
         * @brief Default constructor. Initializes x and y to zero.
         */
        IVector2() : x(0), y(0) {}

        /**
         * @brief Constructs a vector with both x and y set to the provided value.
         * @param p_value The value to set both x and y components to.
         */
        template <typename TOtherType>
        IVector2(const TOtherType &p_value) : x(static_cast<TType>(p_value)), y(static_cast<TType>(p_value)) {}

        /**
         * @brief Constructs a vector from separate x and y values.
         * @param p_x The value for the x component.
         * @param p_y The value for the y component.
         */
        template <typename TTypeA, typename TTypeB>
        IVector2(const TTypeA &p_x, const TTypeB &p_y) : x(static_cast<TType>(p_x)), y(static_cast<TType>(p_y)) {}

        /**
         * @brief Copy constructor from another vector of possibly different type.
         * @param p_other The vector to copy from.
         */
		template <typename TOtherType>
		IVector2(const IVector2<TOtherType> &p_other) : x(static_cast<TType>(p_other.x)), y(static_cast<TType>(p_other.y)) {}

		/**
         * @brief Conversion operator to another vector of possibly different type.
         * @return A new vector of the target type with copied components.
         */
		template <typename TOtherType>
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
			return (std::to_string(x) + " / " + std::to_string(y));
		}

		/**
         * @brief Stream insertion operator to output the vector to an output stream.
         * @param p_os The output stream.
         * @param p_self The vector to output.
         * @return The output stream after insertion.
         */
		friend std::ostream &operator<<(std::ostream &p_os, const IVector2 &p_self)
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
		template <typename TOtherType>
		IVector2<TType> operator+(const IVector2<TOtherType> &p_other) const
		{
			return IVector2<TType>(x + p_other.x, y + p_other.y);
		}

        /**
         * @brief Adds a scalar value to both components of the vector.
         * 
         * @param p_value The scalar value to add.
         * @return The resultant vector after addition.
         */
		template <typename TOtherType>
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
		template <typename TOtherType>
		IVector2<TType> operator-(const IVector2<TOtherType> &p_other) const
		{
			return IVector2<TType>(x - p_other.x, y - p_other.y);
		}

        /**
         * @brief Subtracts a scalar value from both components of the vector.
         * 
         * @param p_value The scalar value to subtract.
         * @return The resultant vector after subtraction.
         */
		template <typename TOtherType>
		IVector2<TType> operator-(const TOtherType &p_value) const
		{
			return IVector2<TType>(x - p_value, y - p_value);
		}

        /**
         * @brief Multiplies this vector by another vector component-wise.
         * 
         * @param p_other The vector to multiply with.
         * @return The resultant vector after multiplication.
         */
		template <typename TOtherType>
		IVector2<TType> operator*(const IVector2<TOtherType> &p_other) const
		{
			return (IVector2<TType>(x * p_other.x, y * p_other.y));
		}

        /**
         * @brief Multiplies both components of this vector by a scalar value.
         * 
         * @param p_value The scalar value to multiply with.
         * @return The resultant vector after multiplication.
         */
		template <typename TOtherType>
		IVector2<TType> operator*(const TOtherType &p_value) const
		{
			return (IVector2<TType>(x * p_value, y * p_value));
		}

        /**
         * @brief Divides this vector by another vector component-wise.
         * 
         * @param p_other The vector to divide by.
         * @return The resultant vector after division.
         */
		template <typename TOtherType>
		IVector2<TType> operator/(const IVector2<TOtherType> &p_other) const
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
		template <typename TOtherType>
		IVector2<TType> operator/(const TOtherType &p_value) const
		{
			if (p_value == 0)
				throwException("Trying to divide by 0");
			return (IVector2<TType>(x / p_value, y / p_value));
		}

		template <typename TOtherType>
		void operator+=(const IVector2<TOtherType> &p_other)
		{
			x += p_other.x;
			y += p_other.y;
		}

		template <typename TOtherType>
		void operator-=(const IVector2<TOtherType> &p_other)
		{
			x -= p_other.x;
			y -= p_other.y;
		}

		template <typename TOtherType>
		void operator*=(const IVector2<TOtherType> &p_other)
		{
			x *= p_other.x;
			y *= p_other.y;
		}

		template <typename TOtherType>
		void operator/=(const IVector2<TOtherType> &p_other)
		{
			if (p_other.x == 0 || p_other.y == 0)
				throwException("Trying to divide by 0");
			x /= p_other.x;
			y /= p_other.y;
		}
		
		template <typename TOtherType>
		bool operator==(const IVector2<TOtherType> &delta) const
		{
			return (x == delta.x && y == delta.y);
		}
		
		template <typename TOtherType>
		bool operator==(const TOtherType &p_value) const
		{
			return (x == p_value && y == p_value);
		}

		template <typename TOtherType>
		bool operator!=(const IVector2<TOtherType> &delta) const
		{
			return (x != delta.x || y != delta.y);
		}

		template <typename TOtherType>
		bool operator!=(const TOtherType &p_value) const
		{
			return (x != p_value || y != p_value);
		}

		template <typename TOtherType>
		bool operator<(const IVector2<TOtherType> &p_other) const
		{
			if (y < p_other.y)
				return (true);
			if (y == p_other.y && x < p_other.x)
				return (true);
			return (false);
		}

		template <typename TOtherType>
		bool operator<(const TOtherType &p_value) const
		{
			if (y < p_value)
				return (true);
			if (y == p_value && x < p_value)
				return (true);
			return (false);
		}

		template <typename TOtherType>
		bool operator>(const IVector2<TOtherType> &p_other) const
		{
			if (y > p_other.y)
				return (true);
			if (y == p_other.y && x > p_other.x)
				return (true);
			return (false);
		}

		template <typename TOtherType>
		bool operator>(const TOtherType &p_value) const
		{
			if (y > p_value)
				return (true);
			if (y == p_value && x > p_value)
				return (true);
			return (false);
		}

		template <typename TOtherType>
		bool operator<=(const IVector2<TOtherType> &p_other) const
		{
			if (y < p_other.y)
				return (true);
			if (y == p_other.y && x <= p_other.x)
				return (true);
			return (false);
		}

		template <typename TOtherType>
		bool operator<=(const TOtherType &p_value) const
		{
			if (y < p_value)
				return (true);
			if (y == p_value && x <= p_value)
				return (true);
			return (false);
		}

		template <typename TOtherType>
		bool operator>=(const TOtherType &p_value) const
		{
			if (y > p_value)
				return (true);
			if (y == p_value && x >= p_value)
				return (true);
			return (false);
		}

		/**
         * @brief Computes the Euclidean norm (length) of the vector.
         * @return The length of the vector.
         */
        float norm() const
        {
            return std::sqrt(x * x + y * y);
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
        float distance(const IVector2 &p_other) const
        {
            return std::sqrt((p_other.x - x) * (p_other.x - x) + (p_other.y - y) * (p_other.y - y));
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
		
		template <typename TOtherType>
		IVector2 cross(const IVector2<TOtherType> &p_other = IVector2(0, 0)) const
		{
			IVector2 result;

			result = IVector2(-(p_other.y - y), p_other.x - x);

			return (result);
		}

		template <typename TOtherType>
		float crossZ(const IVector2<TOtherType> &p_other) const
		{
			return (x * p_other.y - y * p_other.x);
		}

		template <typename TOtherType>
		float dot(const IVector2<TOtherType> &p_other, const IVector2<TOtherType> &p_center = IVector2(0, 0)) const
		{
			float result;

			result = (x - p_center.x) * (p_other.x - p_center.x) + (y - p_center.y) * (p_other.y - p_center.y);

			return (result);
		}

		template <typename TOtherType>
		float angle(const IVector2<TOtherType> &p_other, const IVector2<TOtherType> &p_center = IVector2(0, 0)) const
		{
			float rdot = dot(p_other, p_center);

			rdot = (rdot < -1.0f ? -1.0f : (rdot > 1.0f ? 1.0f : rdot));

			float angle = std::acos(rdot);

			return (spk::radianToDegree(angle));
		}

		template <typename TOtherType>
		IVector2 rotate(const IVector2<TOtherType> &p_center, const float &p_angle) const
		{
			float theta = spk::degreeToRadian(p_angle);

			float pcos = cos(theta);
			float psin = sin(theta);

			IVector2 result;
			result.x = (x - p_center.x) * pcos - (y - p_center.y) * psin;
			result.y = (x - p_center.x) * psin + (y - p_center.y) * pcos;

			return (result + p_center);
		}

		static IVector2 floor(const IVector2 &p_vector)
		{
			IVector2 result;

			result.x = ::floorf(p_vector.x);
			result.y = ::floorf(p_vector.y);

			return (result);
		}

		static IVector2 ceiling(const IVector2 &p_vector)
		{
			IVector2 result;

			result.x = ::ceilf(p_vector.x);
			result.y = ::ceilf(p_vector.y);

			return (result);
		}

		static IVector2 round(const IVector2 &p_vector)
		{
			IVector2 result;

			result.x = ::roundf(p_vector.x);
			result.y = ::roundf(p_vector.y);

			return (result);
		}

		static IVector2 min(const IVector2 &p_min, const IVector2 &p_max)
		{
			return (IVector2(std::min(p_min.x, p_max.x), std::min(p_min.y, p_max.y)));
		}

		static IVector2 max(const IVector2 &p_min, const IVector2 &p_max)
		{
			return (IVector2(std::max(p_min.x, p_max.x), std::max(p_min.y, p_max.y)));
		}

		static bool isInsideRectangle(const IVector2 &p_point, const IVector2 &p_cornerA, const IVector2 &p_cornerB)
		{
			IVector2 tmpMin = IVector2::min(p_cornerA, p_cornerB);
			IVector2 tmpMax = IVector2::max(p_cornerA, p_cornerB);

			if (p_point.x < tmpMin.x || p_point.y < tmpMin.y ||
				p_point.x > tmpMax.x || p_point.y > tmpMax.y)
				return (false);
			return (true);
		}

		static IVector2 lerp(const IVector2 &p_startingPoint, const IVector2 &p_endingPoint, float t)
		{
			return IVector2(
				p_startingPoint.x + (p_endingPoint.x - p_startingPoint.x) * t,
				p_startingPoint.y + (p_endingPoint.y - p_startingPoint.y) * t);
		}

		static IVector2 radianToDegree(const IVector2& p_radians) {
			return IVector2(
				spk::radianToDegree(p_radians.x),
				spk::radianToDegree(p_radians.y));
		}

		static IVector2 degreeToRadian(const IVector2& p_degrees) {
			return IVector2(
				spk::degreeToRadian(p_degrees.x),
				spk::degreeToRadian(p_degrees.y));
		}

		IVector2 clamp(const IVector2& p_lowerValue, const IVector2& p_higherValue)
		{
			return (IVector2(
				std::clamp(x, p_lowerValue.x, p_higherValue.x), 
				std::clamp(y, p_lowerValue.y, p_higherValue.y)
			));
		}
	};

	template <typename TType, typename TValueType, typename = std::enable_if_t<std::is_arithmetic<TValueType>::value>>
	IVector2<TType> operator+(TValueType p_value, const IVector2<TType> &p_point)
	{
		return (IVector2<TType>(p_value, p_value) + p_point);
	};

	template <typename TType, typename TValueType, typename = std::enable_if_t<std::is_arithmetic<TValueType>::value>>
	IVector2<TType> operator-(TValueType p_value, const IVector2<TType> &p_point)
	{
		return (IVector2<TType>(p_value, p_value) - p_point);
	};

	template <typename TType, typename TValueType, typename = std::enable_if_t<std::is_arithmetic<TValueType>::value>>
	IVector2<TType> operator*(TValueType p_value, const IVector2<TType> &p_point)
	{
		return (IVector2<TType>(p_value, p_value) * p_point);
	};

	template <typename TType, typename TValueType, typename = std::enable_if_t<std::is_arithmetic<TValueType>::value>>
	IVector2<TType> operator/(TValueType p_value, const IVector2<TType> &p_point)
	{
		return (IVector2<TType>(p_value, p_value) / p_point);
	};

	template <typename TType>
	std::string to_string(const IVector2<TType> &p_point)
	{
		return (p_point.to_string());
	}

	using Vector2 = IVector2<float>;
	using Vector2Int = IVector2<int>;
	using Vector2UInt = IVector2<unsigned int>;
}