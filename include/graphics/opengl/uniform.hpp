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
	namespace detail
	{
		template <typename TType>
		struct UniformPusher
		{
			static constexpr bool supported = false;
		};

		template <typename TType>
		inline constexpr bool UniformScalar =
			std::is_same_v<TType, float> ||
			std::is_same_v<TType, double> ||
			std::is_same_v<TType, std::int32_t> ||
			std::is_same_v<TType, std::uint32_t>;

		template <>
		struct UniformPusher<bool>
		{
			static constexpr bool supported = true;
			static void push(GLint location, bool value)
			{
				glUniform1i(location, value ? GL_TRUE : GL_FALSE);
			}
		};

		template <typename TType>
			requires UniformScalar<TType>
		struct UniformPusher<TType>
		{
			static constexpr bool supported = true;
			static void push(GLint location, TType value)
			{
				if constexpr (std::is_same_v<TType, float>)
				{
					glUniform1f(location, value);
				}
				else if constexpr (std::is_same_v<TType, double>)
				{
					glUniform1d(location, value);
				}
				else if constexpr (std::is_same_v<TType, std::int32_t>)
				{
					glUniform1i(location, value);
				}
				else
				{
					glUniform1ui(location, value);
				}
			}
		};

		template <typename TType>
			requires UniformScalar<TType>
		struct UniformPusher<TVector2<TType>>
		{
			static constexpr bool supported = true;
			static void push(GLint location, const TVector2<TType> &value)
			{
				if constexpr (std::is_same_v<TType, float>)
				{
					glUniform2f(location, value.x, value.y);
				}
				else if constexpr (std::is_same_v<TType, double>)
				{
					glUniform2d(location, value.x, value.y);
				}
				else if constexpr (std::is_same_v<TType, std::int32_t>)
				{
					glUniform2i(location, value.x, value.y);
				}
				else
				{
					glUniform2ui(location, value.x, value.y);
				}
			}
		};

		template <typename TType>
			requires UniformScalar<TType>
		struct UniformPusher<TVector3<TType>>
		{
			static constexpr bool supported = true;
			static void push(GLint location, const TVector3<TType> &value)
			{
				if constexpr (std::is_same_v<TType, float>)
				{
					glUniform3f(location, value.x, value.y, value.z);
				}
				else if constexpr (std::is_same_v<TType, double>)
				{
					glUniform3d(location, value.x, value.y, value.z);
				}
				else if constexpr (std::is_same_v<TType, std::int32_t>)
				{
					glUniform3i(location, value.x, value.y, value.z);
				}
				else
				{
					glUniform3ui(location, value.x, value.y, value.z);
				}
			}
		};

		template <typename TType>
			requires UniformScalar<TType>
		struct UniformPusher<TVector4<TType>>
		{
			static constexpr bool supported = true;
			static void push(GLint location, const TVector4<TType> &value)
			{
				if constexpr (std::is_same_v<TType, float>)
				{
					glUniform4f(location, value.x, value.y, value.z, value.w);
				}
				else if constexpr (std::is_same_v<TType, double>)
				{
					glUniform4d(location, value.x, value.y, value.z, value.w);
				}
				else if constexpr (std::is_same_v<TType, std::int32_t>)
				{
					glUniform4i(location, value.x, value.y, value.z, value.w);
				}
				else
				{
					glUniform4ui(location, value.x, value.y, value.z, value.w);
				}
			}
		};

		template <std::size_t TSize>
		struct UniformPusher<Matrix<TSize, TSize>>
		{
			static constexpr bool supported = TSize >= 2 && TSize <= 4;

			static void push(GLint location, const Matrix<TSize, TSize> &value)
			{
				static_assert(supported, "Unsupported OpenGL uniform matrix type");
				std::array<float, TSize * TSize> data{};
				for (std::size_t column = 0; column < TSize; ++column)
				{
					for (std::size_t row = 0; row < TSize; ++row)
					{
						data[column * TSize + row] = value[column][row];
					}
				}

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
	}

	template <typename TType>
	class Uniform
	{
	public:
		using Data = std::remove_cvref_t<TType>;

	private:
		static_assert(detail::UniformPusher<Data>::supported, "Unsupported OpenGL uniform type");

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
			static_assert(detail::UniformPusher<Value>::supported, "Unsupported OpenGL uniform type");
			detail::UniformPusher<Value>::push(location, value);
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
