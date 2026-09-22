#pragma once

#include <GL/glew.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "math/matrix.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "math/vector4.hpp"

namespace spk
{
	template <typename TType>
	class Uniform
	{
	public:
		using Data = std::remove_cvref_t<TType>;

	private:
		struct Pusher
		{
			template <typename TValue>
			static void push(GLint, const TValue &) = delete;

			static void push(GLint location, bool value)
			{
				glUniform1i(location, value ? GL_TRUE : GL_FALSE);
			}

			static void push(GLint location, float value)
			{
				glUniform1f(location, value);
			}

			static void push(GLint location, double value)
			{
				glUniform1d(location, value);
			}

			static void push(GLint location, std::int32_t value)
			{
				glUniform1i(location, value);
			}

			static void push(GLint location, std::uint32_t value)
			{
				glUniform1ui(location, value);
			}

			template <typename TValue>
			static constexpr bool Numeric =
				std::is_same_v<TValue, float> ||
				std::is_same_v<TValue, double> ||
				std::is_same_v<TValue, std::int32_t> ||
				std::is_same_v<TValue, std::uint32_t>;

			template <typename TValue>
				requires Numeric<TValue>
			static void push(GLint location, const TVector2<TValue> &value)
			{
				if constexpr (std::is_same_v<TValue, float>)
				{
					glUniform2f(location, value.x, value.y);
				}
				else if constexpr (std::is_same_v<TValue, double>)
				{
					glUniform2d(location, value.x, value.y);
				}
				else if constexpr (std::is_same_v<TValue, std::int32_t>)
				{
					glUniform2i(location, value.x, value.y);
				}
				else
				{
					glUniform2ui(location, value.x, value.y);
				}
			}

			template <typename TValue>
				requires Numeric<TValue>
			static void push(GLint location, const TVector3<TValue> &value)
			{
				if constexpr (std::is_same_v<TValue, float>)
				{
					glUniform3f(location, value.x, value.y, value.z);
				}
				else if constexpr (std::is_same_v<TValue, double>)
				{
					glUniform3d(location, value.x, value.y, value.z);
				}
				else if constexpr (std::is_same_v<TValue, std::int32_t>)
				{
					glUniform3i(location, value.x, value.y, value.z);
				}
				else
				{
					glUniform3ui(location, value.x, value.y, value.z);
				}
			}

			template <typename TValue>
				requires Numeric<TValue>
			static void push(GLint location, const TVector4<TValue> &value)
			{
				if constexpr (std::is_same_v<TValue, float>)
				{
					glUniform4f(location, value.x, value.y, value.z, value.w);
				}
				else if constexpr (std::is_same_v<TValue, double>)
				{
					glUniform4d(location, value.x, value.y, value.z, value.w);
				}
				else if constexpr (std::is_same_v<TValue, std::int32_t>)
				{
					glUniform4i(location, value.x, value.y, value.z, value.w);
				}
				else
				{
					glUniform4ui(location, value.x, value.y, value.z, value.w);
				}
			}

			template <std::size_t TSize>
			[[nodiscard]] static std::array<float, TSize * TSize> _matrixData(const Matrix<TSize, TSize> &matrix)
			{
				std::array<float, TSize * TSize> result{};
				for (std::size_t column = 0; column < TSize; ++column)
				{
					for (std::size_t row = 0; row < TSize; ++row)
					{
						result[column * TSize + row] = matrix[column][row];
					}
				}
				return result;
			}

			template <std::size_t TSize>
				requires(TSize >= 2 && TSize <= 4)
			static void push(GLint location, const Matrix<TSize, TSize> &value)
			{
				const auto data = _matrixData(value);
				if constexpr (TSize == 2)
				{
					glUniformMatrix2fv(location, 1, GL_FALSE, data.data());
				}
				else if constexpr (TSize == 3)
				{
					glUniformMatrix3fv(location, 1, GL_FALSE, data.data());
				}
				else
				{
					glUniformMatrix4fv(location, 1, GL_FALSE, data.data());
				}
			}
		};

		static constexpr bool Supported = requires(GLint location, const Data &data) {
			Pusher::push(location, data);
		};
		static_assert(Supported, "Unsupported OpenGL uniform type");

		std::string _name;
		Data _data{};

		[[nodiscard]] GLint _location() const
		{
			GLint program = 0;
			glGetIntegerv(GL_CURRENT_PROGRAM, &program);
			if (program == 0)
			{
				throw std::logic_error("Cannot activate a Uniform without an active Program");
			}
			return glGetUniformLocation(static_cast<GLuint>(program), _name.c_str());
		}

		template <typename TValue>
		static void _push(GLint location, const TValue &value)
		{
			using Value = std::remove_cvref_t<TValue>;
			static_assert(
				requires(GLint target, const Value &data) {
					Pusher::push(target, data);
				},
				"Unsupported OpenGL uniform type");
			Pusher::push(location, value);
		}

	public:
		explicit Uniform(std::string name) :
			_name(std::move(name))
		{
			if (_name.empty())
			{
				throw std::invalid_argument("Uniform name cannot be empty");
			}
		}

		Uniform(std::string name, const Data &data) :
			Uniform(std::move(name))
		{
			_data = data;
		}

		[[nodiscard]] const std::string &name() const noexcept
		{
			return _name;
		}

		[[nodiscard]] Data &data() noexcept
		{
			return _data;
		}

		[[nodiscard]] const Data &data() const noexcept
		{
			return _data;
		}

		void setData(const Data &data)
		{
			_data = data;
		}

		void activate() const
		{
			const GLint location = _location();
			if (location != -1)
			{
				_push(location, _data);
			}
		}
	};
}
