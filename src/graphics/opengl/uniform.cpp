#include "graphics/opengl/uniform.hpp"

#include <array>
#include <cstddef>

namespace
{
	template <std::size_t TSize>
	[[nodiscard]] std::array<float, TSize * TSize> matrixData(const spk::Matrix<TSize, TSize> &matrix)
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
}

namespace spk
{
	GLint Uniform::_location() const
	{
		GLint program = 0;
		glGetIntegerv(GL_CURRENT_PROGRAM, &program);
		if (program == 0)
		{
			throw std::logic_error("Cannot activate a Uniform without an active Program");
		}
		return glGetUniformLocation(static_cast<GLuint>(program), _name.c_str());
	}

	void Uniform::_activate(GLint location, bool value)
	{
		glUniform1i(location, value ? GL_TRUE : GL_FALSE);
	}

	void Uniform::_activate(GLint location, std::int32_t value)
	{
		glUniform1i(location, value);
	}

	void Uniform::_activate(GLint location, std::uint32_t value)
	{
		glUniform1ui(location, value);
	}

	void Uniform::_activate(GLint location, float value)
	{
		glUniform1f(location, value);
	}

	void Uniform::_activate(GLint location, const Vector2 &value)
	{
		glUniform2f(location, value.x, value.y);
	}

	void Uniform::_activate(GLint location, const Vector2Int &value)
	{
		glUniform2i(location, value.x, value.y);
	}

	void Uniform::_activate(GLint location, const Vector2UInt &value)
	{
		glUniform2ui(location, value.x, value.y);
	}

	void Uniform::_activate(GLint location, const Vector3 &value)
	{
		glUniform3f(location, value.x, value.y, value.z);
	}

	void Uniform::_activate(GLint location, const Vector3Int &value)
	{
		glUniform3i(location, value.x, value.y, value.z);
	}

	void Uniform::_activate(GLint location, const Vector3UInt &value)
	{
		glUniform3ui(location, value.x, value.y, value.z);
	}

	void Uniform::_activate(GLint location, const Vector4 &value)
	{
		glUniform4f(location, value.x, value.y, value.z, value.w);
	}

	void Uniform::_activate(GLint location, const Vector4Int &value)
	{
		glUniform4i(location, value.x, value.y, value.z, value.w);
	}

	void Uniform::_activate(GLint location, const Vector4UInt &value)
	{
		glUniform4ui(location, value.x, value.y, value.z, value.w);
	}

	void Uniform::_activate(GLint location, const Matrix2x2 &value)
	{
		const auto data = matrixData(value);
		glUniformMatrix2fv(location, 1, GL_FALSE, data.data());
	}

	void Uniform::_activate(GLint location, const Matrix3x3 &value)
	{
		const auto data = matrixData(value);
		glUniformMatrix3fv(location, 1, GL_FALSE, data.data());
	}

	void Uniform::_activate(GLint location, const Matrix4x4 &value)
	{
		const auto data = matrixData(value);
		glUniformMatrix4fv(location, 1, GL_FALSE, data.data());
	}

	void Uniform::activate() const
	{
		const GLint location = _location();
		if (location == -1)
		{
			return;
		}
		std::visit(
			[location](const auto &value) {
				_activate(location, value);
			},
			_value);
	}
}
