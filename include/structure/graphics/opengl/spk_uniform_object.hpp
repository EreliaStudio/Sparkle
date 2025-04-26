#pragma once

#include <GL/glew.h>

#include <GL/gl.h>

#include "structure/container/spk_data_buffer.hpp"
#include "utils/spk_string_utils.hpp"
#include <string>

#include "structure/math/spk_matrix.hpp"
#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"
#include "structure/math/spk_vector4.hpp"

namespace spk::OpenGL
{
	class UniformObject
	{
	public:
		enum class Type {
			None,
			Float,
			Bool,
			Int,
			UInt,
			Vector2,
			Vector3,
			Vector4,
			Vector2Int,
			Vector3Int,
			Vector4Int,
			Vector2UInt,
			Vector3UInt,
			Vector4UInt,
			Matrix2x2,
			Matrix3x3,
			Matrix4x4
		};

	private:
		std::string _name;
		GLint _location;
		Type _type;
		spk::DataBuffer _data;
		mutable std::mutex _mutex;
		bool _needUpdate = false;

		void _retrieveLocation()
		{
			GLint prog = 0;
			glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
			glUseProgram(prog);

			_location = glGetUniformLocation(prog, _name.c_str());

			if (_location == -1)
			{
				throw std::runtime_error("Uniform '" + _name + "' not found in shader program.");
			}
		}

	public:
		UniformObject() :
			_name("Unnamed uniform"),
			_location(-1),
			_type(Type::None)
		{
		}

		UniformObject(const std::wstring &name, const Type &p_type) :
			_location(-1),
			_name(spk::StringUtils::wstringToString(name)),
			_type(p_type)
		{
			size_t size = 0;
			switch (_type)
			{
			case Type::Float:
				size = sizeof(float);
				break;
			case Type::Bool:
				size = sizeof(GLboolean);
				break;
			case Type::Int:
				size = sizeof(GLint);
				break;
			case Type::UInt:
				size = sizeof(GLuint);
				break;

			case Type::Vector2:
				size = sizeof(spk::Vector2);
				break;
			case Type::Vector3:
				size = sizeof(spk::Vector3);
				break;
			case Type::Vector4:
				size = sizeof(spk::Vector4);
				break;

			case Type::Matrix2x2:
				size = sizeof(spk::Matrix2x2);
				break;
			case Type::Matrix3x3:
				size = sizeof(spk::Matrix3x3);
				break;
			case Type::Matrix4x4:
				size = sizeof(spk::Matrix4x4);
				break;

			default:
				throw std::runtime_error("Unsupported type in UniformObject constructor.");
			}
			_data.resize(size);
		}

		UniformObject(const UniformObject &p_other)
		{
			std::lock_guard<std::mutex> lock(p_other._mutex);
			_name = p_other._name;
			_location = p_other._location;
			_type = p_other._type;
			_data = p_other._data;
			_needUpdate = p_other._needUpdate;
		}

		UniformObject(UniformObject &&p_other) noexcept
		{
			std::lock_guard<std::mutex> lock(p_other._mutex);

			_name = std::move(p_other._name);
			_location = p_other._location;
			_type = p_other._type;
			_data = std::move(p_other._data);
			_needUpdate = p_other._needUpdate;

			p_other._name = "Unnamed";
			p_other._location = -1;
			p_other._type = Type::None;
			p_other._data.clear();
		}

		UniformObject &operator=(const UniformObject &p_other)
		{
			if (this != &p_other)
			{
				std::lock(_mutex, p_other._mutex);

				std::lock_guard<std::mutex> lhs_lock(_mutex, std::adopt_lock);
				std::lock_guard<std::mutex> rhs_lock(p_other._mutex, std::adopt_lock);

				_name = p_other._name;
				_location = p_other._location;
				_type = p_other._type;
				_data = p_other._data;
				_needUpdate = p_other._needUpdate;
			}
			return *this;
		}

		UniformObject &operator=(UniformObject &&p_other) noexcept
		{
			if (this != &p_other)
			{
				std::lock(_mutex, p_other._mutex);
				std::lock_guard<std::mutex> lhs_lock(_mutex, std::adopt_lock);
				std::lock_guard<std::mutex> rhs_lock(p_other._mutex, std::adopt_lock);
				_name = std::move(p_other._name);
				_location = p_other._location;
				_type = p_other._type;
				_data = std::move(p_other._data);
				_needUpdate = p_other._needUpdate;

				p_other._name = "Unnamed";
				p_other._location = -1;
				p_other._type = Type::None;
				p_other._data.clear();
			}
			return *this;
		}

		~UniformObject() = default;

		template <typename TType>
		UniformObject &operator=(const TType &value)
		{
			std::lock_guard<std::mutex> lock(_mutex);

			size_t size = sizeof(value);
			if (_data.size() != size)
			{
				throw std::runtime_error("Size mismatch for uniform [" + _name + "].");
			}
			std::memcpy(_data.data(), &value, size);
			_needUpdate = true;
			return *this;
		}

		void activate()
		{
			std::lock_guard<std::mutex> lock(_mutex);

			if (_location == -1)
			{
				_retrieveLocation();
				_needUpdate = true;
			}

			if (_needUpdate == true)
			{
				switch (_type)
				{
				case Type::Float:
				{
					glUniform1f(_location, *reinterpret_cast<const float *>(_data.data()));
					break;
				}
				case Type::Bool:
				{
					glUniform1i(_location, *reinterpret_cast<const GLboolean *>(_data.data()));
					break;
				}
				case Type::Int:
				{
					glUniform1i(_location, *reinterpret_cast<const GLint *>(_data.data()));
					break;
				}
				case Type::UInt:
				{
					glUniform1ui(_location, *reinterpret_cast<const GLuint *>(_data.data()));
					break;
				}
				case Type::Vector2:
				{
					glUniform2fv(_location, 1, reinterpret_cast<const float *>(_data.data()));
					break;
				}
				case Type::Vector3:
				{
					glUniform3fv(_location, 1, reinterpret_cast<const float *>(_data.data()));
					break;
				}
				case Type::Vector4:
				{
					glUniform4fv(_location, 1, reinterpret_cast<const float *>(_data.data()));
					break;
				}
				case Type::Matrix2x2:
				{
					glUniformMatrix2fv(_location, 1, GL_FALSE, reinterpret_cast<const float *>(_data.data()));
					break;
				}
				case Type::Matrix3x3:
				{
					glUniformMatrix3fv(_location, 1, GL_FALSE, reinterpret_cast<const float *>(_data.data()));
					break;
				}
				case Type::Matrix4x4:
				{
					glUniformMatrix4fv(_location, 1, GL_FALSE, reinterpret_cast<const float *>(_data.data()));
					break;
				}
				default:
					throw std::runtime_error("Unsupported uniform type in UniformObject::activate.");
				}

				_needUpdate = false;
			}
		}
	};
}