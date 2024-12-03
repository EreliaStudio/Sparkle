#pragma once

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <type_traits>
#include <unordered_map>
#include <sstream>
#include <algorithm>

#include "structure/math/spk_math.hpp"
#include "structure/math/spk_vector2.hpp"

namespace spk
{
	/**
	 * @brief A template struct for representing a 3-dimensional vector.
	 *
	 * This struct provides basic vector operations and supports different arithmetic types.
	 *
	 * @tparam TType The arithmetic type of the vector components (e.g., int, float, double).
	 *
	 * Example usage:
	 * @code
	 * spk::IVector3<float> vec(1.0f, 2.0f, 3.0f);
	 * std::cout << "Vector: (" << vec.x << ", " << vec.y << ", " << vec.z << ")\n";
	 * @endcode
	 */
	template <typename TType>
	struct IVector3
	{
		static_assert(std::is_arithmetic<TType>::value, "TType must be an arithmetic type.");

		TType x, y, z;

		/**
		 * @brief Default constructor that initializes x, y, and z to zero.
		 */
		IVector3() : x(0), y(0), z(0) {}

		/**
		 * @brief Constructs a vector with all components set to the same scalar value.
		 *
		 * @tparam UType The type of the scalar value, must be arithmetic.
		 * @param p_scalar The scalar value to initialize all components.
		 */
		template <typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
		IVector3(UType p_scalar) :
			x(static_cast<TType>(p_scalar)),
			y(static_cast<TType>(p_scalar)),
			z(static_cast<TType>(p_scalar))
		{}

		/**
		 * @brief Constructs a vector with specified values for each component.
		 *
		 * @tparam UType The type of the x component, must be arithmetic.
		 * @tparam VType The type of the y component, must be arithmetic.
		 * @tparam WType The type of the z component, must be arithmetic.
		 * @param p_xVal The value for the x component.
		 * @param p_yVal The value for the y component.
		 * @param p_zVal The value for the z component.
		 */
		template <typename UType, typename VType, typename WType,
				  typename = std::enable_if_t<std::is_arithmetic<UType>::value && std::is_arithmetic<VType>::value && std::is_arithmetic<WType>::value>>
		IVector3(UType p_xVal, VType p_yVal, WType p_zVal) :
			x(static_cast<TType>(p_xVal)),
			y(static_cast<TType>(p_yVal)),
			z(static_cast<TType>(p_zVal))
		{}

		/**
		 * @brief Copy constructor that creates a new vector by copying another vector's components.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The other vector to copy from.
		 */
		template <typename UType>
		IVector3(const IVector3<UType> &p_other) : x(static_cast<TType>(p_other.x)), y(static_cast<TType>(p_other.y)), z(static_cast<TType>(p_other.z)) {}

		/**
		 * @brief Converts this vector to another vector type.
		 *
		 * @tparam UType The target type for the vector components.
		 * @return A new vector with components cast to UType.
		 */
		template <typename UType>
		explicit operator IVector3<UType>() const
		{
			return IVector3<UType>(static_cast<UType>(x), static_cast<UType>(y), static_cast<UType>(z));
		}

		/**
		 * @brief Copy assignment operator that assigns another vector's components to this vector.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The other vector to assign from.
		 * @return A reference to this vector after assignment.
		 */
		template <typename UType>
		IVector3 &operator=(const IVector3<UType> &p_other)
		{
			x = static_cast<TType>(p_other.x);
			y = static_cast<TType>(p_other.y);
			z = static_cast<TType>(p_other.z);

			return (*this);
		}

		/**
		 * @brief Stream insertion operator for wide output streams.
		 *
		 * This friend function allows the vector to be inserted into wide output streams (std::wostream).
		 *
		 * @param p_os The wide output stream.
		 * @param p_vec The vector to insert into the stream.
		 * @return Reference to the wide output stream.
		 *
		 * @code
		 * spk::IVector3<float> vec(1.0f, 2.0f, 3.0f);
		 * std::wcout << vec; // Outputs: (1.0, 2.0, 3.0)
		 * @endcode
		 */
		friend std::wostream &operator<<(std::wostream &p_os, const IVector3 &p_vec)
		{
			p_os << L"(" << p_vec.x << L", " << p_vec.y << L", " << p_vec.z << L")";
			return p_os;
		}

		/**
		 * @brief Stream insertion operator for standard output streams.
		 *
		 * This friend function allows the vector to be inserted into standard output streams (std::ostream).
		 *
		 * @param p_os The standard output stream.
		 * @param p_vec The vector to insert into the stream.
		 * @return Reference to the standard output stream.
		 *
		 * @code
		 * spk::IVector3<float> vec(1.0f, 2.0f, 3.0f);
		 * std::cout << vec; // Outputs: (1.0, 2.0, 3.0)
		 * @endcode
		 */
		friend std::ostream &operator<<(std::ostream &p_os, const IVector3 &p_vec)
		{
			p_os << "(" << p_vec.x << ", " << p_vec.y << ", " << p_vec.z << ")";
			return p_os;
		}

		/**
		 * @brief Converts the vector to a wide string representation.
		 *
		 * @return A `std::wstring` representing the vector in the format "(x, y, z)".
		 *
		 * @code
		 * spk::IVector3<float> vec(1.0f, 2.0f, 3.0f);
		 * std::wstring wstr = vec.to_wstring(); // wstr = L"(1.0, 2.0, 3.0)"
		 * @endcode
		 */
		std::wstring to_wstring() const
		{
			std::wstringstream wss;
			wss << *this;
			return wss.str();
		}

		/**
		 * @brief Converts the vector to a standard string representation.
		 *
		 * @return A `std::string` representing the vector in the format "(x, y, z)".
		 *
		 * @code
		 * spk::IVector3<float> vec(1.0f, 2.0f, 3.0f);
		 * std::string str = vec.to_string(); // str = "(1.0, 2.0, 3.0)"
		 * @endcode
		 */
		std::string to_string() const
		{
			std::stringstream ss;
			ss << *this;
			return ss.str();
		} /**
		   * @brief Equality comparison operator between two vectors.
		   *
		   * Compares this vector with another vector of potentially different type for equality.
		   * For floating-point types, it uses an epsilon value to account for precision errors.
		   *
		   * @tparam UType The type of the other vector's components.
		   * @param p_other The other vector to compare with.
		   * @return `true` if the vectors are equal within the epsilon range or exactly equal for non-floating types; otherwise, `false`.
		   */
		template <typename UType>
		bool operator==(const IVector3<UType> &p_other) const
		{
			if constexpr (std::is_floating_point<TType>::value)
			{
				constexpr TType epsilon = static_cast<TType>(1e-5);
				return std::fabs(x - static_cast<TType>(p_other.x)) < epsilon &&
					   std::fabs(y - static_cast<TType>(p_other.y)) < epsilon &&
					   std::fabs(z - static_cast<TType>(p_other.z)) < epsilon;
			}
			else
			{
				return x == static_cast<TType>(p_other.x) &&
					   y == static_cast<TType>(p_other.y) &&
					   z == static_cast<TType>(p_other.z);
			}
		}

		/**
		 * @brief Equality comparison operator between a vector and a scalar.
		 *
		 * Compares each component of this vector with a scalar value for equality.
		 * For floating-point types, it uses an epsilon value to account for precision errors.
		 *
		 * @tparam UType The type of the scalar value.
		 * @param p_scalar The scalar value to compare with.
		 * @return `true` if all components are equal to the scalar within the epsilon range or exactly equal for non-floating types; otherwise, `false`.
		 */
		template <typename UType>
		bool operator==(UType p_scalar) const
		{
			if constexpr (std::is_floating_point<TType>::value)
			{
				constexpr TType epsilon = static_cast<TType>(1e-5);
				return std::fabs(x - static_cast<TType>(p_scalar)) < epsilon &&
					   std::fabs(y - static_cast<TType>(p_scalar)) < epsilon &&
					   std::fabs(z - static_cast<TType>(p_scalar)) < epsilon;
			}
			else
			{
				return x == static_cast<TType>(p_scalar) &&
					   y == static_cast<TType>(p_scalar) &&
					   z == static_cast<TType>(p_scalar);
			}
		}

		/**
		 * @brief Inequality comparison operator between two vectors.
		 *
		 * Determines if this vector is not equal to another vector of potentially different type.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The other vector to compare with.
		 * @return `true` if the vectors are not equal; otherwise, `false`.
		 */
		template <typename UType>
		bool operator!=(const IVector3<UType> &p_other) const
		{
			return !(*this == p_other);
		}

		/**
		 * @brief Inequality comparison operator between a vector and a scalar.
		 *
		 * Determines if any component of this vector is not equal to the scalar value.
		 *
		 * @tparam UType The type of the scalar value.
		 * @param p_scalar The scalar value to compare with.
		 * @return `true` if any component is not equal to the scalar; otherwise, `false`.
		 */
		template <typename UType>
		bool operator!=(UType p_scalar) const
		{
			return !(*this == p_scalar);
		}

		/**
		 * @brief Less-than comparison operator between two vectors.
		 *
		 * Compares this vector with another vector lexicographically.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The other vector to compare with.
		 * @return `true` if this vector is lexicographically less than the other vector; otherwise, `false`.
		 */
		template <typename UType>
		bool operator<(const IVector3<UType> &p_other) const
		{
			return x < p_other.x ||
				   (x == p_other.x && y < p_other.y) ||
				   (x == p_other.x && y == p_other.y && z < p_other.z);
		}

		/**
		 * @brief Greater-than comparison operator between two vectors.
		 *
		 * Compares this vector with another vector lexicographically.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The other vector to compare with.
		 * @return `true` if this vector is lexicographically greater than the other vector; otherwise, `false`.
		 */
		template <typename UType>
		bool operator>(const IVector3<UType> &p_other) const
		{
			return x > p_other.x ||
				   (x == p_other.x && y > p_other.y) ||
				   (x == p_other.x && y == p_other.y && z > p_other.z);
		}

		/**
		 * @brief Less-than or equal-to comparison operator between two vectors.
		 *
		 * Determines if this vector is lexicographically less than or equal to another vector.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The other vector to compare with.
		 * @return `true` if this vector is lexicographically less than or equal to the other vector; otherwise, `false`.
		 */
		template <typename UType>
		bool operator<=(const IVector3<UType> &p_other) const
		{
			return !(*this > p_other);
		}

		/**
		 * @brief Greater-than or equal-to comparison operator between two vectors.
		 *
		 * Determines if this vector is lexicographically greater than or equal to another vector.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The other vector to compare with.
		 * @return `true` if this vector is lexicographically greater than or equal to the other vector; otherwise, `false`.
		 */
		template <typename UType>
		bool operator>=(const IVector3<UType> &p_other) const
		{
			return !(*this < p_other);
		}

		/**
		 * @brief Less-than comparison operator between a vector and a scalar.
		 *
		 * Determines if all components of this vector are less than the scalar value.
		 *
		 * @tparam UType The type of the scalar value.
		 * @param p_scalar The scalar value to compare with.
		 * @return `true` if all components are less than the scalar; otherwise, `false`.
		 */
		template <typename UType>
		bool operator<(UType p_scalar) const
		{
			return x < p_scalar && y < p_scalar && z < p_scalar;
		}

		/**
		 * @brief Greater-than comparison operator between a vector and a scalar.
		 *
		 * Determines if all components of this vector are greater than the scalar value.
		 *
		 * @tparam UType The type of the scalar value.
		 * @param p_scalar The scalar value to compare with.
		 * @return `true` if all components are greater than the scalar; otherwise, `false`.
		 */
		template <typename UType>
		bool operator>(UType p_scalar) const
		{
			return x > p_scalar && y > p_scalar && z > p_scalar;
		}

		/**
		 * @brief Less-than or equal-to comparison operator between a vector and a scalar.
		 *
		 * Determines if all components of this vector are less than or equal to the scalar value.
		 *
		 * @tparam UType The type of the scalar value.
		 * @param p_scalar The scalar value to compare with.
		 * @return `true` if all components are less than or equal to the scalar; otherwise, `false`.
		 */
		template <typename UType>
		bool operator<=(UType p_scalar) const
		{
			return x <= p_scalar && y <= p_scalar && z <= p_scalar;
		}

		/**
		 * @brief Greater-than or equal-to comparison operator between a vector and a scalar.
		 *
		 * Determines if all components of this vector are greater than or equal to the scalar value.
		 *
		 * @tparam UType The type of the scalar value.
		 * @param p_scalar The scalar value to compare with.
		 * @return `true` if all components are greater than or equal to the scalar; otherwise, `false`.
		 */
		template <typename UType>
		bool operator>=(UType p_scalar) const
		{
			return x >= p_scalar && y >= p_scalar && z >= p_scalar;
		}

		/**
		 * @brief Unary minus operator.
		 *
		 * Returns a new vector with each component negated.
		 *
		 * @return A new `IVector3<TType>` instance with components (-x, -y, -z).
		 */
		IVector3<TType> operator-() const
		{
			return IVector3<TType>(-x, -y, -z);
		}

		/**
		 * @brief Addition operator with another vector.
		 *
		 * Adds this vector to another vector component-wise.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The other vector to add.
		 * @return A new `IVector3<TType>` instance representing the sum of the two vectors.
		 */
		template <typename UType>
		IVector3<TType> operator+(const IVector3<UType> &p_other) const
		{
			return IVector3<TType>(x + p_other.x, y + p_other.y, z + p_other.z);
		}

		/**
		 * @brief Subtraction operator with another vector.
		 *
		 * Subtracts another vector from this vector component-wise.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The other vector to subtract.
		 * @return A new `IVector3<TType>` instance representing the difference of the two vectors.
		 */
		template <typename UType>
		IVector3<TType> operator-(const IVector3<UType> &p_other) const
		{
			return IVector3<TType>(x - p_other.x, y - p_other.y, z - p_other.z);
		}

		/**
		 * @brief Multiplication operator with another vector.
		 *
		 * Multiplies this vector with another vector component-wise.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The other vector to multiply.
		 * @return A new `IVector3<TType>` instance representing the product of the two vectors.
		 */
		template <typename UType>
		IVector3<TType> operator*(const IVector3<UType> &p_other) const
		{
			return IVector3<TType>(x * p_other.x, y * p_other.y, z * p_other.z);
		}

		/**
		 * @brief Division operator with another vector.
		 *
		 * Divides this vector by another vector component-wise.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The other vector to divide by.
		 * @return A new `IVector3<TType>` instance representing the division of the two vectors.
		 * @throws `std::runtime_error` if any component of the other vector is zero.
		 */
		template <typename UType>
		IVector3<TType> operator/(const IVector3<UType> &p_other) const
		{
			if (p_other.x == 0 || p_other.y == 0 || p_other.z == 0)
			{
				throw std::runtime_error("Division by zero");
			}
			return IVector3<TType>(x / p_other.x, y / p_other.y, z / p_other.z);
		}

		/**
		 * @brief Addition operator with a scalar.
		 *
		 * Adds a scalar value to each component of this vector.
		 *
		 * @tparam UType The type of the scalar value.
		 * @param p_scalar The scalar value to add.
		 * @return A new `IVector3<TType>` instance representing the sum of the vector and the scalar.
		 */
		template <typename UType>
		IVector3<TType> operator+(UType p_scalar) const
		{
			return IVector3<TType>(x + p_scalar, y + p_scalar, z + p_scalar);
		}

		/**
		 * @brief Subtraction operator with a scalar.
		 *
		 * Subtracts a scalar value from each component of this vector.
		 *
		 * @tparam UType The type of the scalar value.
		 * @param p_scalar The scalar value to subtract.
		 * @return A new `IVector3<TType>` instance representing the difference of the vector and the scalar.
		 */
		template <typename UType>
		IVector3<TType> operator-(UType p_scalar) const
		{
			return IVector3<TType>(x - p_scalar, y - p_scalar, z - p_scalar);
		}

		/**
		 * @brief Multiplication operator with a scalar.
		 *
		 * Multiplies each component of this vector by a scalar value.
		 *
		 * @tparam UType The type of the scalar value.
		 * @param p_scalar The scalar value to multiply.
		 * @return A new `IVector3<TType>` instance representing the product of the vector and the scalar.
		 */
		template <typename UType>
		IVector3<TType> operator*(UType p_scalar) const
		{
			return IVector3<TType>(x * p_scalar, y * p_scalar, z * p_scalar);
		}

		/**
		 * @brief Division operator with a scalar.
		 *
		 * Divides each component of this vector by a scalar value.
		 *
		 * @tparam UType The type of the scalar value.
		 * @param p_scalar The scalar value to divide by.
		 * @return A new `IVector3<TType>` instance representing the division of the vector by the scalar.
		 * @throws `std::runtime_error` if the scalar value is zero.
		 */
		template <typename UType>
		IVector3<TType> operator/(UType p_scalar) const
		{
			if (p_scalar == 0)
			{
				throw std::runtime_error("Division by zero");
			}
			return IVector3<TType>(x / p_scalar, y / p_scalar, z / p_scalar);
		}

		/**
		 * @brief Compound assignment operator for vector addition.
		 *
		 * Adds another vector to this vector component-wise and updates this vector.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The other vector to add.
		 * @return Reference to this vector after addition.
		 */
		template <typename UType>
		IVector3<TType> &operator+=(const IVector3<UType> &p_other)
		{
			x += p_other.x;
			y += p_other.y;
			z += p_other.z;
			return *this;
		}

		/**
		 * @brief Compound assignment operator for vector subtraction.
		 *
		 * Subtracts another vector from this vector component-wise and updates this vector.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The other vector to subtract.
		 * @return Reference to this vector after subtraction.
		 */
		template <typename UType>
		IVector3<TType> &operator-=(const IVector3<UType> &p_other)
		{
			x -= p_other.x;
			y -= p_other.y;
			z -= p_other.z;
			return *this;
		}

		/**
		 * @brief Compound assignment operator for vector multiplication.
		 *
		 * Multiplies this vector with another vector component-wise and updates this vector.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The other vector to multiply.
		 * @return Reference to this vector after multiplication.
		 */
		template <typename UType>
		IVector3<TType> &operator*=(const IVector3<UType> &p_other)
		{
			x *= p_other.x;
			y *= p_other.y;
			z *= p_other.z;
			return *this;
		}

		/**
		 * @brief Compound assignment operator for vector division.
		 *
		 * Divides this vector by another vector component-wise and updates this vector.
		 *
		 * @tparam UType The type of the other vector's components.
		 * @param p_other The other vector to divide by.
		 * @return Reference to this vector after division.
		 * @throws `std::runtime_error` if any component of the other vector is zero.
		 */
		template <typename UType>
		IVector3<TType> &operator/=(const IVector3<UType> &p_other)
		{
			if (p_other.x == 0 || p_other.y == 0 || p_other.z == 0)
			{
				throw std::runtime_error("Division by zero");
			}
			x /= p_other.x;
			y /= p_other.y;
			z /= p_other.z;
			return *this;
		}

		/**
		 * @brief Compound assignment operator for scalar addition.
		 *
		 * Adds a scalar value to each component of this vector and updates this vector.
		 *
		 * @tparam UType The type of the scalar value.
		 * @param p_scalar The scalar value to add.
		 * @return Reference to this vector after addition.
		 */
		template <typename UType>
		IVector3<TType> &operator+=(UType p_scalar)
		{
			x += static_cast<TType>(p_scalar);
			y += static_cast<TType>(p_scalar);
			z += static_cast<TType>(p_scalar);
			return *this;
		}

		/**
		 * @brief Compound assignment operator for scalar subtraction.
		 *
		 * Subtracts a scalar value from each component of this vector and updates this vector.
		 *
		 * @tparam UType The type of the scalar value.
		 * @param p_scalar The scalar value to subtract.
		 * @return Reference to this vector after subtraction.
		 */
		template <typename UType>
		IVector3<TType> &operator-=(UType p_scalar)
		{
			x -= static_cast<TType>(p_scalar);
			y -= static_cast<TType>(p_scalar);
			z -= static_cast<TType>(p_scalar);
			return *this;
		}

		/**
		 * @brief Compound assignment operator for scalar multiplication.
		 *
		 * Multiplies each component of this vector by a scalar value and updates this vector.
		 *
		 * @tparam UType The type of the scalar value.
		 * @param p_scalar The scalar value to multiply.
		 * @return Reference to this vector after multiplication.
		 */
		template <typename UType>
		IVector3<TType> &operator*=(UType p_scalar)
		{
			x *= static_cast<TType>(p_scalar);
			y *= static_cast<TType>(p_scalar);
			z *= static_cast<TType>(p_scalar);
			return *this;
		}

		/**
		 * @brief Compound assignment operator for scalar division.
		 *
		 * Divides each component of this vector by a scalar value and updates this vector.
		 *
		 * @tparam UType The type of the scalar value.
		 * @param p_scalar The scalar value to divide by.
		 * @return Reference to this vector after division.
		 * @throws `std::runtime_error` if the scalar value is zero.
		 */
		template <typename UType>
		IVector3<TType> &operator/=(UType p_scalar)
		{
			if (p_scalar == 0)
			{
				throw std::runtime_error("Division by zero");
			}

			x /= static_cast<TType>(p_scalar);
			y /= static_cast<TType>(p_scalar);
			z /= static_cast<TType>(p_scalar);
			return *this;
		}

		/**
		 * @brief Calculates the distance between this vector and another vector.
		 *
		 * Computes the Euclidean distance between this vector and the specified other vector.
		 *
		 * @param p_other The other vector to calculate the distance to.
		 * @return The Euclidean distance as a `float`.
		 */
		float distance(const IVector3<TType> &p_other) const
		{
			return static_cast<float>(std::sqrt(std::pow(p_other.x - x, 2) + std::pow(p_other.y - y, 2) + std::pow(p_other.z - z, 2)));
		}

		/**
		 * @brief Calculates the norm (magnitude) of the vector.
		 *
		 * Computes the Euclidean norm of this vector.
		 *
		 * @return The norm as a `float`.
		 */
		float norm() const
		{
			return static_cast<float>(std::sqrt(x * x + y * y + z * z));
		}

		/**
		 * @brief Normalizes the vector.
		 *
		 * Returns a new vector with the same direction but a norm of 1.
		 *
		 * @return A new `IVector3<float>` instance representing the normalized vector.
		 * @throws `std::runtime_error` if the vector has a norm of zero.
		 */
		IVector3<float> normalize() const
		{
			float len = norm();
			if (len == 0)
				throw std::runtime_error("Can't calculate a norm for a vector of length 0");
			return IVector3<float>(x / len, y / len, z / len);
		}

		/**
		 * @brief Calculates the cross product with another vector.
		 *
		 * Computes the cross product of this vector and the specified other vector.
		 *
		 * @param p_other The other vector to compute the cross product with.
		 * @return A new `IVector3<TType>` instance representing the cross product.
		 */
		IVector3<TType> cross(const IVector3<TType> &p_other) const
		{
			return IVector3<TType>(
				y * p_other.z - z * p_other.y,
				z * p_other.x - x * p_other.z,
				x * p_other.y - y * p_other.x);
		}

		/**
		 * @brief Rotates the vector by the specified rotation values.
		 *
		 * Applies a rotation to this vector based on the provided rotation angles around each axis.
		 *
		 * @param p_rotationValues The rotation angles (in degrees) around the X, Y, and Z axes.
		 * @return A new `IVector3<TType>` instance representing the rotated vector.
		 */
		IVector3<TType> rotate(const IVector3<TType> &p_rotationValues) const
		{
			TType cos_x = std::cos(spk::degreeToRadian(static_cast<float>(p_rotationValues.x)));
			TType sin_x = std::sin(spk::degreeToRadian(static_cast<float>(p_rotationValues.x)));
			TType cos_y = std::cos(spk::degreeToRadian(static_cast<float>(p_rotationValues.y)));
			TType sin_y = std::sin(spk::degreeToRadian(static_cast<float>(p_rotationValues.y)));
			TType cos_z = std::cos(spk::degreeToRadian(static_cast<float>(p_rotationValues.z)));
			TType sin_z = std::sin(spk::degreeToRadian(static_cast<float>(p_rotationValues.z)));

			IVector3<TType> result;
			result.x = cos_y * cos_z * x + (sin_x * sin_y * cos_z - cos_x * sin_z) * y + (cos_x * sin_y * cos_z + sin_x * sin_z) * z;
			result.y = cos_y * sin_z * x + (sin_x * sin_y * sin_z + cos_x * cos_z) * y + (cos_x * sin_y * sin_z - sin_x * cos_z) * z;
			result.z = -sin_y * x + sin_x * cos_y * y + cos_x * cos_y * z;

			return result;
		}

		/**
		 * @brief Calculates the dot product with another vector.
		 *
		 * Computes the dot product of this vector and the specified other vector.
		 *
		 * @param p_other The other vector to compute the dot product with.
		 * @return The dot product as a `TType`.
		 */
		TType dot(const IVector3<TType> &p_other) const
		{
			return x * p_other.x + y * p_other.y + z * p_other.z;
		}

		/**
		 * @brief Inverts the vector.
		 *
		 * Returns a new vector with each component negated.
		 *
		 * @return A new `IVector3<TType>` instance representing the inverted vector.
		 */
		IVector3 inverse() const
		{
			return IVector3(-x, -y, -z);
		}

		/**
		 * @brief Retrieves the XY components of the vector.
		 *
		 * Returns a 2-dimensional vector containing the X and Y components of this vector.
		 *
		 * @return A new `IVector2<TType>` instance containing the X and Y components.
		 */
		IVector2<TType> xy() const
		{
			return IVector2<TType>(x, y);
		}

		/**
		 * @brief Retrieves the XZ components of the vector.
		 *
		 * Returns a 2-dimensional vector containing the X and Z components of this vector.
		 *
		 * @return A new `IVector2<TType>` instance containing the X and Z components.
		 */
		IVector2<TType> xz() const
		{
			return IVector2<TType>(x, z);
		}

		/**
		 * @brief Retrieves the YZ components of the vector.
		 *
		 * Returns a 2-dimensional vector containing the Y and Z components of this vector.
		 *
		 * @return A new `IVector2<TType>` instance containing the Y and Z components.
		 */
		IVector2<TType> yz() const
		{
			return IVector2<TType>(y, z);
		}

		/**
		 * @brief Converts radians to degrees for a vector.
		 *
		 * Converts each component of the provided vector from radians to degrees.
		 *
		 * @param p_radians The vector containing angles in radians.
		 * @return A new `IVector3` instance with each component converted to degrees.
		 */
		static IVector3 radianToDegree(const IVector3 &p_radians)
		{
			return IVector3(
				spk::radianToDegree(p_radians.x),
				spk::radianToDegree(p_radians.y),
				spk::radianToDegree(p_radians.z));
		}

		/**
		 * @brief Converts degrees to radians for a vector.
		 *
		 * Converts each component of the provided vector from degrees to radians.
		 *
		 * @param p_degrees The vector containing angles in degrees.
		 * @return A new `IVector3` instance with each component converted to radians.
		 */
		static IVector3 degreeToRadian(const IVector3 &p_degrees)
		{
			return IVector3(
				spk::degreeToRadian(p_degrees.x),
				spk::degreeToRadian(p_degrees.y),
				spk::degreeToRadian(p_degrees.z));
		}

		/**
		 * @brief Clamps the vector within the specified lower and upper bounds.
		 *
		 * Restricts each component of this vector to lie within the corresponding lower and upper bounds.
		 *
		 * @param p_lowerValue The vector representing the lower bounds.
		 * @param p_higherValue The vector representing the upper bounds.
		 * @return A new `IVector3<TType>` instance with each component clamped.
		 */
		IVector3 clamp(const IVector3 &p_lowerValue, const IVector3 &p_higherValue)
		{
			return IVector3(
				std::clamp(x, p_lowerValue.x, p_higherValue.x),
				std::clamp(y, p_lowerValue.y, p_higherValue.y),
				std::clamp(z, p_lowerValue.z, p_higherValue.z));
		}

		/**
		 * @brief Applies the floor function to each component of a vector.
		 *
		 * Returns a new vector with each component rounded down to the nearest integer.
		 *
		 * @tparam TOtherType The type of the input vector's components.
		 * @param p_vector The vector to apply the floor function to.
		 * @return A new `IVector3` instance with each component floored.
		 */
		template <typename TOtherType>
		static IVector3 floor(const IVector3<TOtherType> &p_vector)
		{
			IVector3 result;
			result.x = std::floor(p_vector.x);
			result.y = std::floor(p_vector.y);
			result.z = std::floor(p_vector.z);
			return result;
		}

		/**
		 * @brief Applies the ceiling function to each component of a vector.
		 *
		 * Returns a new vector with each component rounded up to the nearest integer.
		 *
		 * @tparam TOtherType The type of the input vector's components.
		 * @param p_vector The vector to apply the ceiling function to.
		 * @return A new `IVector3` instance with each component ceiled.
		 */
		template <typename TOtherType>
		static IVector3 ceiling(const IVector3<TOtherType> &p_vector)
		{
			IVector3 result;
			result.x = std::ceil(p_vector.x);
			result.y = std::ceil(p_vector.y);
			result.z = std::ceil(p_vector.z);
			return result;
		}

		/**
		 * @brief Applies the round function to each component of a vector.
		 *
		 * Returns a new vector with each component rounded to the nearest integer.
		 *
		 * @tparam TOtherType The type of the input vector's components.
		 * @param p_vector The vector to apply the round function to.
		 * @return A new `IVector3` instance with each component rounded.
		 */
		template <typename TOtherType>
		static IVector3 round(const IVector3<TOtherType> &p_vector)
		{
			IVector3 result;
			result.x = std::round(p_vector.x);
			result.y = std::round(p_vector.y);
			result.z = std::round(p_vector.z);
			return result;
		}

		/**
		 * @brief Finds the component-wise minimum of two vectors.
		 *
		 * Returns a new vector containing the minimum values of each corresponding component from the two input vectors.
		 *
		 * @param p_min The first vector.
		 * @param p_max The second vector.
		 * @return A new `IVector3` instance with each component set to the minimum of the corresponding components.
		 */
		static IVector3 min(const IVector3 &p_min, const IVector3 &p_max)
		{
			return IVector3(std::min(p_min.x, p_max.x), std::min(p_min.y, p_max.y), std::min(p_min.z, p_max.z));
		}

		/**
		 * @brief Finds the component-wise maximum of two vectors.
		 *
		 * Returns a new vector containing the maximum values of each corresponding component from the two input vectors.
		 *
		 * @param p_min The first vector.
		 * @param p_max The second vector.
		 * @return A new `IVector3` instance with each component set to the maximum of the corresponding components.
		 */
		static IVector3 max(const IVector3 &p_min, const IVector3 &p_max)
		{
			return IVector3(std::max(p_min.x, p_max.x), std::max(p_min.y, p_max.y), std::max(p_min.z, p_max.z));
		}

		/**
		 * @brief Linearly interpolates between two vectors.
		 *
		 * Computes the linear interpolation between the starting and ending vectors based on the parameter `t`.
		 *
		 * @param p_startingPoint The starting vector.
		 * @param p_endingPoint The ending vector.
		 * @param t The interpolation parameter (typically between 0 and 1).
		 * @return A new `IVector3` instance representing the interpolated vector.
		 */
		static IVector3 lerp(const IVector3 &p_startingPoint, const IVector3 &p_endingPoint, float t)
		{
			return IVector3(
				p_startingPoint.x + (p_endingPoint.x - p_startingPoint.x) * t,
				p_startingPoint.y + (p_endingPoint.y - p_startingPoint.y) * t,
				p_startingPoint.z + (p_endingPoint.z - p_startingPoint.z) * t);
		}
	};

	/**
	 * @brief Alias for a 3-dimensional vector with floating-point components.
	 */
	using Vector3 = IVector3<float_t>;

	/**
	 * @brief Alias for a 3-dimensional vector with 32-bit integer components.
	 */
	using Vector3Int = IVector3<int32_t>;

	/**
	 * @brief Alias for a 3-dimensional vector with 32-bit unsigned integer components.
	 */
	using Vector3UInt = IVector3<uint32_t>;
}
/**
 * @brief Addition operator for a scalar and a vector.
 * 
 * Adds a scalar value to each component of the provided vector.
 * 
 * @tparam TType The type of the vector's components.
 * @tparam UType The type of the scalar value, must be an arithmetic type.
 * @param p_scalar The scalar value to add.
 * @param p_vec The vector to which the scalar is added.
 * @return A new `IVector3<TType>` instance representing the result of the addition.
 */
template <typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector3<TType> operator+(UType p_scalar, const spk::IVector3<TType> &p_vec)
{
	return spk::IVector3<TType>(p_scalar + p_vec.x, p_scalar + p_vec.y, p_scalar + p_vec.z);
}

/**
 * @brief Subtraction operator for a scalar and a vector.
 * 
 * Subtracts each component of the provided vector from a scalar value.
 * 
 * @tparam TType The type of the vector's components.
 * @tparam UType The type of the scalar value, must be an arithmetic type.
 * @param p_scalar The scalar value from which the vector is subtracted.
 * @param p_vec The vector to subtract from the scalar.
 * @return A new `IVector3<TType>` instance representing the result of the subtraction.
 */
template <typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector3<TType> operator-(UType p_scalar, const spk::IVector3<TType> &p_vec)
{
	return spk::IVector3<TType>(p_scalar - p_vec.x, p_scalar - p_vec.y, p_scalar - p_vec.z);
}

/**
 * @brief Multiplication operator for a scalar and a vector.
 * 
 * Multiplies a scalar value with each component of the provided vector.
 * 
 * @tparam TType The type of the vector's components.
 * @tparam UType The type of the scalar value, must be an arithmetic type.
 * @param p_scalar The scalar value to multiply.
 * @param p_vec The vector to be multiplied by the scalar.
 * @return A new `IVector3<TType>` instance representing the result of the multiplication.
 */
template <typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector3<TType> operator*(UType p_scalar, const spk::IVector3<TType> &p_vec)
{
	return spk::IVector3<TType>(p_scalar * p_vec.x, p_scalar * p_vec.y, p_scalar * p_vec.z);
}

/**
 * @brief Division operator for a scalar and a vector.
 * 
 * Divides a scalar value by each component of the provided vector.
 * 
 * @tparam TType The type of the vector's components.
 * @tparam UType The type of the scalar value, must be an arithmetic type.
 * @param p_scalar The scalar value to be divided.
 * @param p_vec The vector by which the scalar is divided.
 * @return A new `IVector3<TType>` instance representing the result of the division.
 * @throws `std::runtime_error` if any component of the vector is zero to prevent division by zero.
 */
template <typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector3<TType> operator/(UType p_scalar, const spk::IVector3<TType> &p_vec)
{
	if (p_vec.x == 0 || p_vec.y == 0 || p_vec.z == 0)
	{
		throw std::runtime_error("Division by zero");
	}
	return spk::IVector3<TType>(p_scalar / p_vec.x, p_scalar / p_vec.y, p_scalar / p_vec.z);
}

namespace std
{
	/**
	 * @brief Specialization of `std::hash` for `spk::IVector3`.
	 * 
	 * Enables hashing of `IVector3` instances for use in unordered containers like `std::unordered_map` and `std::unordered_set`.
	 * 
	 * @tparam TType The type of the vector's components.
	 */
	template <typename TType>
	struct hash<spk::IVector3<TType>>
	{
		/**
		 * @brief Computes the hash value for an `IVector3` instance.
		 * 
		 * Combines the hash values of each component using bitwise operations.
		 * 
		 * @param p_vec The vector to hash.
		 * @return A `size_t` representing the hash value of the vector.
		 */
		size_t operator()(const spk::IVector3<TType> &p_vec) const
		{
			return hash<TType>()(p_vec.x) ^ (hash<TType>()(p_vec.y) << 1) ^ (hash<TType>()(p_vec.z) << 2);
		}
	};
}
