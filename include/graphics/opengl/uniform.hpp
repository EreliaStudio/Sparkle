#pragma once

#include <GL/glew.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include "math/matrix.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "math/vector4.hpp"

namespace spk
{
	class Uniform
	{
	private:
		using Value = std::variant<
			bool,
			std::int32_t,
			std::uint32_t,
			float,
			Vector2,
			Vector2Int,
			Vector2UInt,
			Vector3,
			Vector3Int,
			Vector3UInt,
			Vector4,
			Vector4Int,
			Vector4UInt,
			Matrix2x2,
			Matrix3x3,
			Matrix4x4>;

		template <typename TType>
		using Decayed = std::remove_cvref_t<TType>;

		template <typename TType>
		static constexpr bool Supported =
			std::is_same_v<Decayed<TType>, bool> ||
			std::is_same_v<Decayed<TType>, std::int32_t> ||
			std::is_same_v<Decayed<TType>, std::uint32_t> ||
			std::is_same_v<Decayed<TType>, float> ||
			std::is_same_v<Decayed<TType>, Vector2> ||
			std::is_same_v<Decayed<TType>, Vector2Int> ||
			std::is_same_v<Decayed<TType>, Vector2UInt> ||
			std::is_same_v<Decayed<TType>, Vector3> ||
			std::is_same_v<Decayed<TType>, Vector3Int> ||
			std::is_same_v<Decayed<TType>, Vector3UInt> ||
			std::is_same_v<Decayed<TType>, Vector4> ||
			std::is_same_v<Decayed<TType>, Vector4Int> ||
			std::is_same_v<Decayed<TType>, Vector4UInt> ||
			std::is_same_v<Decayed<TType>, Matrix2x2> ||
			std::is_same_v<Decayed<TType>, Matrix3x3> ||
			std::is_same_v<Decayed<TType>, Matrix4x4>;

		std::string _name;
		Value _value;

		[[nodiscard]] GLint _location() const;
		static void _activate(GLint location, bool value);
		static void _activate(GLint location, std::int32_t value);
		static void _activate(GLint location, std::uint32_t value);
		static void _activate(GLint location, float value);
		static void _activate(GLint location, const Vector2 &value);
		static void _activate(GLint location, const Vector2Int &value);
		static void _activate(GLint location, const Vector2UInt &value);
		static void _activate(GLint location, const Vector3 &value);
		static void _activate(GLint location, const Vector3Int &value);
		static void _activate(GLint location, const Vector3UInt &value);
		static void _activate(GLint location, const Vector4 &value);
		static void _activate(GLint location, const Vector4Int &value);
		static void _activate(GLint location, const Vector4UInt &value);
		static void _activate(GLint location, const Matrix2x2 &value);
		static void _activate(GLint location, const Matrix3x3 &value);
		static void _activate(GLint location, const Matrix4x4 &value);

	public:
		template <typename TType>
			requires(Supported<TType>)
		Uniform(std::string name, const TType &value) :
			_name(std::move(name)),
			_value(value)
		{
			if (_name.empty())
			{
				throw std::invalid_argument("Uniform name cannot be empty");
			}
		}

		[[nodiscard]] const std::string &name() const noexcept
		{
			return _name;
		}

		template <typename TType>
			requires(Supported<TType>)
		void setData(const TType &value)
		{
			using Type = Decayed<TType>;
			auto *stored = std::get_if<Type>(&_value);
			if (stored == nullptr)
			{
				throw std::logic_error("Uniform type cannot be changed");
			}
			*stored = value;
		}

		template <typename TType>
			requires(Supported<TType>)
		[[nodiscard]] Decayed<TType> &cast()
		{
			using Type = Decayed<TType>;
			auto *stored = std::get_if<Type>(&_value);
			if (stored == nullptr)
			{
				throw std::logic_error("Uniform type differs from the requested type");
			}
			return *stored;
		}

		template <typename TType>
			requires(Supported<TType>)
		[[nodiscard]] const Decayed<TType> &cast() const
		{
			using Type = Decayed<TType>;
			const auto *stored = std::get_if<Type>(&_value);
			if (stored == nullptr)
			{
				throw std::logic_error("Uniform type differs from the requested type");
			}
			return *stored;
		}

		void activate() const;
	};
}
