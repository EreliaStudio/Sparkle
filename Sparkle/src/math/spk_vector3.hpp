#pragma once

#include <stdexcept>
#include <string>

#include "math/spk_vector2.hpp"
#include "miscellaneous/JSON/spk_JSON_object.hpp"

namespace spk
{
	template <typename TType>
	struct IVector3
	{
		TType x;
		TType y;
		TType z;

		IVector3() : x(0), y(0), z(0) {}

		IVector3(const TType &p_value) : x(p_value), y(p_value), z(p_value) {}

		IVector3(const IVector2<TType> &p_other, const TType &p_value) : x(p_other.x), y(p_other.y), z(p_value) {}

		IVector3(const IVector3<TType> &p_other) : x(p_other.x), y(p_other.y), z(p_other.z) {}

		IVector3(const TType &p_x, const TType &p_y, const TType &p_z) : x(p_x), y(p_y), z(p_z) {}

		IVector3(const spk::JSON::Object& p_object)
		{
			if constexpr (std::is_floating_point<TType>::value)
			{
				x = p_object["x"].as<double>();
				y = p_object["y"].as<double>();
				z = p_object["z"].as<double>();
			}
			else
			{
				x = p_object["x"].as<long>();
				y = p_object["y"].as<long>();
				z = p_object["z"].as<long>();
			}
		}

		operator IVector3<TType>()
		{
			return (IVector3<TType>(static_cast<TType>(x), static_cast<TType>(y), static_cast<TType>(z)));
		}

		IVector3 operator-() const
		{
        	return (IVector3(-x, -y, -z));
    	}

		friend std::ostream &operator<<(std::ostream &p_os, const IVector3 &p_self)
		{
			p_os << p_self.x << " / " << p_self.y << " / " << p_self.z;
			return (p_os);
		}

		std::string to_string() const
		{
			return (std::to_string(x) + L" / " + std::to_string(y) + L" / " + std::to_string(z));
		}

		IVector3<TType> operator+(const IVector3 &p_other) const
		{
			return (IVector3<TType>(x + p_other.x, y + p_other.y, z + p_other.z));
		}

		IVector3<TType> operator-(const IVector3 &p_other) const
		{
			return (IVector3<TType>(x - p_other.x, y - p_other.y, z - p_other.z));
		}

		IVector3<TType> operator*(const IVector3 &p_other) const
		{
			return (IVector3<TType>(x * p_other.x, y * p_other.y, z * p_other.z));
		}

		IVector3<TType> operator/(const IVector3 &p_other) const
		{
			if (p_other.x == 0 || p_other.y == 0 || p_other.z == 0)
				throwException("Trying to divide by 0");
			return (IVector3<TType>(x / p_other.x, y / p_other.y, z / p_other.z));
		}

		void operator+=(const IVector3 &p_other)
		{
			x += p_other.x;
			y += p_other.y;
			z += p_other.z;
		}

		void operator-=(const IVector3 &p_other)
		{
			x -= p_other.x;
			y -= p_other.y;
			z -= p_other.z;
		}

		void operator*=(const IVector3 &p_other)
		{
			x *= p_other.x;
			y *= p_other.y;
			z *= p_other.z;
		}

		void operator/=(const IVector3 &p_other)
		{
			if (p_other.x == 0 || p_other.y == 0 || p_other.z == 0)
				throwException("Trying to divide by 0");
			x /= p_other.x;
			y /= p_other.y;
			z /= p_other.z;
		}

		bool operator==(const TType &p_other) const
		{
			return (x == static_cast<TType>(p_other) && y == static_cast<TType>(p_other) && z == static_cast<TType>(p_other));
		}

		bool operator!=(const TType &p_other) const
		{
			return (x != static_cast<TType>(p_other) || y != static_cast<TType>(p_other) || z != static_cast<TType>(p_other));
		}

		bool operator<(const TType &p_other) const
		{
			if (z < static_cast<TType>(p_other))
				return (true);
			if (z == static_cast<TType>(p_other) && y < static_cast<TType>(p_other))
				return (true);
			if (z == static_cast<TType>(p_other) && y == static_cast<TType>(p_other) && x < static_cast<TType>(p_other))
				return (true);
			return (false);
		}

		bool operator>(const TType &p_other) const
		{
			if (z > static_cast<TType>(p_other))
				return (true);
			if (z == static_cast<TType>(p_other) && y > static_cast<TType>(p_other))
				return (true);
			if (z == static_cast<TType>(p_other) && y == static_cast<TType>(p_other) && x > static_cast<TType>(p_other))
				return (true);
			return (false);
		}

		bool operator<=(const TType &p_other) const
		{
			if (this->operator==(p_other) == true || this->operator<(p_other) == true)
				return (true);
			return (false);
		}

		bool operator>=(const TType &p_other) const
		{
			if (this->operator==(p_other) == true || this->operator>(p_other) == true)
				return (true);
			return (false);
		}

		bool operator==(const IVector3<TType> &p_other) const
		{
			return ((x == static_cast<TType>(p_other.x) && y == static_cast<TType>(p_other.y) && z == static_cast<TType>(p_other.z) ? true : false));
		}

		bool operator!=(const IVector3<TType> &p_other) const
		{
			return ((x == static_cast<TType>(p_other.x) && y == static_cast<TType>(p_other.y) && z == static_cast<TType>(p_other.z) ? false : true));
		}

		bool operator<(const IVector3<TType> &p_other) const
		{
			if (z < static_cast<TType>(p_other.z))
				return (true);
			if (z == static_cast<TType>(p_other.z) && y < static_cast<TType>(p_other.y))
				return (true);
			if (z == static_cast<TType>(p_other.z) && y == static_cast<TType>(p_other.y) && x < static_cast<TType>(p_other.x))
				return (true);
			return (false);
		}

		bool operator>(const IVector3<TType> &p_other) const
		{
			if (z > static_cast<TType>(p_other.z))
				return (true);
			if (z == static_cast<TType>(p_other.z) && y > static_cast<TType>(p_other.y))
				return (true);
			if (z == static_cast<TType>(p_other.z) && y == static_cast<TType>(p_other.y) && x > static_cast<TType>(p_other.x))
				return (true);
			return (false);
		}

		bool operator<=(const IVector3<TType> &p_other) const
		{
			if (this->operator==(p_other) == true || this->operator<(p_other) == true)
				return (true);
			return (false);
		}

		bool operator>=(const IVector3<TType> &p_other) const
		{
			if (this->operator==(p_other) == true || this->operator>(p_other) == true)
				return (true);
			return (false);
		}

		static IVector3 lerp(const IVector3 &a, const IVector3 &b, float t)
		{
			return IVector3(
				a.x + (b.x - a.x) * t,
				a.y + (b.y - a.y) * t,
				a.z + (b.z - a.z) * t);
		}

		float distance(const IVector3<TType> &p_other) const
		{
			return (static_cast<float>(::sqrt(pow(p_other.x - x, 2) + pow(p_other.y - y, 2) + pow(p_other.z - z, 2))));
		}

		float norm() const
		{
			return (sqrt(pow(x, 2.0f) + pow(y, 2.0f) + pow(z, 2.0f)));
		}

		IVector3<float> normalize() const
		{
			IVector3<float> result;

			float tmpnorm = norm();

			if (tmpnorm == 0)
				throwException("Try to divide by 0");

			result.x = x / tmpnorm;
			result.y = y / tmpnorm;
			result.z = z / tmpnorm;

			return (result);
		}

		IVector3<TType> cross(const IVector3<TType> &p_other) const
		{
			return IVector3<TType>(
				y * p_other.z - z * p_other.y,
				z * p_other.x - x * p_other.z,
				x * p_other.y - y * p_other.x);
		}

		IVector3<TType> rotate(const IVector3<TType> &p_rotationValues) const
		{
			TType x_rad = spk::degreeToRadian(p_rotationValues.x);
			TType y_rad = spk::degreeToRadian(p_rotationValues.y);
			TType z_rad = spk::degreeToRadian(p_rotationValues.z);

			TType cos_x = cos(x_rad);
			TType sin_x = sin(x_rad);
			TType cos_y = cos(y_rad);
			TType sin_y = sin(y_rad);
			TType cos_z = cos(z_rad);
			TType sin_z = sin(z_rad);

			IVector3<TType> result;
			result.x = cos_y * cos_z * x + (sin_x * sin_y * cos_z - cos_x * sin_z) * y + (cos_x * sin_y * cos_z + sin_x * sin_z) * z;
			result.y = cos_y * sin_z * x + (sin_x * sin_y * sin_z + cos_x * cos_z) * y + (cos_x * sin_y * sin_z - sin_x * cos_z) * z;
			result.z = -sin_y * x + sin_x * cos_y * y + cos_x * cos_y * z;

			return result;
		}

		TType dot(const IVector3<TType> &p_other) const
		{
			return x * p_other.x + y * p_other.y + z * p_other.z;
		}

		IVector3 projectOnPlane(const IVector3 &p_planeNormal, const IVector3 &p_planePoint) const
		{
			IVector3 pointToPlane = *this - p_planePoint;
			float distance = pointToPlane.dot(p_planeNormal);
			return (*this - p_planeNormal * distance);
		}

		IVector2<TType> removeY()
		{
			return (IVector2<TType>(x, z));
		}

		IVector3 inverse() const
		{
			return (IVector3(-x, -y, -z));
		}

		static IVector3 radianToDegree(const IVector3& radians) {
			return IVector3(
					spk::radianToDegree(radians.x),
					spk::radianToDegree(radians.y),
					spk::radianToDegree(radians.z)
				);
		}

		static IVector3 degreeToRadian(const IVector3& degrees) {
			return IVector3(
					spk::degreeToRadian(degrees.x),
					spk::degreeToRadian(degrees.y),
					spk::degreeToRadian(degrees.z)
				);
		}

		IVector3 clamp(const IVector3& p_lowerValue, const IVector3& p_higherValue)
		{
			return (IVector3(
				std::clamp(x, p_lowerValue.x, p_higherValue.x), 
				std::clamp(y, p_lowerValue.y, p_higherValue.y), 
				std::clamp(z, p_lowerValue.z, p_higherValue.z)
			));
		}
	};

	template <typename TType, typename TValueType, typename = std::enable_if_t<std::is_arithmetic<TValueType>::value>>
	IVector3<TType> operator+(TValueType p_value, const IVector3<TType> &p_point)
	{
		return (IVector3<TType>(p_value) + p_point);
	};

	template <typename TType, typename TValueType, typename = std::enable_if_t<std::is_arithmetic<TValueType>::value>>
	IVector3<TType> operator-(TValueType p_value, const IVector3<TType> &p_point)
	{
		return (IVector3<TType>(p_value) - p_point);
	};

	template <typename TType, typename TValueType, typename = std::enable_if_t<std::is_arithmetic<TValueType>::value>>
	IVector3<TType> operator*(TValueType p_value, const IVector3<TType> &p_point)
	{
		return (IVector3<TType>(p_value) * p_point);
	};

	template <typename TType, typename TValueType, typename = std::enable_if_t<std::is_arithmetic<TValueType>::value>>
	IVector3<TType> operator/(TValueType p_value, const IVector3<TType> &p_point)
	{
		return (IVector3<TType>(p_value) / p_point);
	};

	template <typename TType>
	std::string to_string(const IVector3<TType> &p_point)
	{
		return (p_point.to_string());
	}

	using Vector3 = IVector3<float>;
	using Vector3Int = IVector3<int>;
	using Vector3UInt = IVector3<unsigned int>;
};
