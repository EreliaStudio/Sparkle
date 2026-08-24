#include "graphics/opengl/program.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>

namespace spk
{
	class Program::Instance final : public GPUResource::Instance
	{
	public:
		GLuint identifier = 0;

		~Instance() override
		{
			if (identifier != 0)
			{
				glDeleteProgram(identifier);
			}
		}
	};

	GLenum Program::_openGLPrimitive(Primitive primitive)
	{
		switch (primitive)
		{
		case Primitive::Points:
			return GL_POINTS;
		case Primitive::Lines:
			return GL_LINES;
		case Primitive::LineStrip:
			return GL_LINE_STRIP;
		case Primitive::Triangles:
			return GL_TRIANGLES;
		case Primitive::TriangleStrip:
			return GL_TRIANGLE_STRIP;
		default:
			throw std::logic_error("Unsupported primitive");
		}
	}

	GLenum Program::_openGLIndexType(IndexBuffer::Type type) noexcept
	{
		switch (type)
		{
		case IndexBuffer::Type::UnsignedByte:
			return GL_UNSIGNED_BYTE;
		case IndexBuffer::Type::UnsignedShort:
			return GL_UNSIGNED_SHORT;
		case IndexBuffer::Type::UnsignedInt:
			return GL_UNSIGNED_INT;
		}
		return GL_UNSIGNED_INT;
	}

	std::string Program::_shaderLog(GLuint shader)
	{
		GLint length = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		std::string result(static_cast<std::size_t>(std::max(length, 0)), '\0');
		if (length > 0)
		{
			glGetShaderInfoLog(shader, length, nullptr, result.data());
		}
		return result;
	}

	std::string Program::_programLog(GLuint program)
	{
		GLint length = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
		std::string result(static_cast<std::size_t>(std::max(length, 0)), '\0');
		if (length > 0)
		{
			glGetProgramInfoLog(program, length, nullptr, result.data());
		}
		return result;
	}

	GLuint Program::_compileShader(GLenum type, const std::string &source)
	{
		const GLuint shader = glCreateShader(type);
		if (shader == 0)
		{
			throw std::runtime_error("Failed to create OpenGL shader");
		}
		const char *sourcePointer = source.c_str();
		glShaderSource(shader, 1, &sourcePointer, nullptr);
		glCompileShader(shader);
		GLint success = GL_FALSE;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (success == GL_TRUE)
		{
			return shader;
		}
		const auto log = _shaderLog(shader);
		glDeleteShader(shader);
		throw std::runtime_error("OpenGL shader compilation failed:\n" + log);
	}

	GLuint Program::_linkProgram(GLuint vertexShader, GLuint fragmentShader)
	{
		const GLuint program = glCreateProgram();
		if (program == 0)
		{
			throw std::runtime_error("Failed to create OpenGL program");
		}
		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);
		glLinkProgram(program);
		GLint success = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (success == GL_TRUE)
		{
			return program;
		}
		const auto log = _programLog(program);
		glDeleteProgram(program);
		throw std::runtime_error("OpenGL program linking failed:\n" + log);
	}

	GLuint Program::_buildProgram(const std::string &vertexSource, const std::string &fragmentSource)
	{
		const GLuint vertexShader = _compileShader(GL_VERTEX_SHADER, vertexSource);
		GLuint fragmentShader = 0;
		try
		{
			fragmentShader = _compileShader(GL_FRAGMENT_SHADER, fragmentSource);
			const GLuint program = _linkProgram(vertexShader, fragmentShader);
			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);
			return program;
		} catch (...)
		{
			glDeleteShader(vertexShader);
			if (fragmentShader != 0)
			{
				glDeleteShader(fragmentShader);
			}
			throw;
		}
	}

	void Program::_validateGLCount(std::size_t count)
	{
		if (count > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()))
		{
			throw std::overflow_error("Draw count exceeds OpenGL GLsizei range");
		}
	}

	GPUResource::Kind Program::_kind() const noexcept
	{
		return GPUResource::Kind::Program;
	}

	std::unique_ptr<GPUResource::Instance> Program::_create(RenderContext &) const
	{
		return std::make_unique<Instance>();
	}

	void Program::_synchronize(GPUResource::Instance &base, RenderContext &) const
	{
		if (!isValid())
		{
			throw std::logic_error("Cannot synchronize an invalid Program");
		}

		auto &instance = static_cast<Instance &>(base);
		const GLuint identifier = _buildProgram(_vertexShaderSource, _fragmentShaderSource);

		try
		{
			_applyUniformBlockBindings(identifier);
			_applyShaderStorageBlockBindings(identifier);
			_applySamplerBindings(identifier);
		} catch (...)
		{
			glDeleteProgram(identifier);
			throw;
		}

		if (instance.identifier != 0)
		{
			glDeleteProgram(instance.identifier);
		}

		instance.identifier = identifier;
	}

	void Program::_bind(GPUResource::Instance &base, RenderContext &) const
	{
		glUseProgram(static_cast<Instance &>(base).identifier);
	}

	Program::Program(std::string vertexShaderSource, std::string fragmentShaderSource) :
		_vertexShaderSource(std::move(vertexShaderSource)),
		_fragmentShaderSource(std::move(fragmentShaderSource))
	{
		validate();
	}

	void Program::setSources(std::string vertexShaderSource, std::string fragmentShaderSource)
	{
		if (_vertexShaderSource == vertexShaderSource && _fragmentShaderSource == fragmentShaderSource)
		{
			return;
		}
		_vertexShaderSource = std::move(vertexShaderSource);
		_fragmentShaderSource = std::move(fragmentShaderSource);
		validate();
	}

	bool Program::isValid() const noexcept
	{
		return !_vertexShaderSource.empty() && !_fragmentShaderSource.empty();
	}

	void Program::renderRaw(Primitive primitive, std::size_t firstVertex, std::size_t vertexCount) const
	{
		_validateGLCount(vertexCount);
		if (firstVertex > static_cast<std::size_t>(std::numeric_limits<GLint>::max()))
		{
			throw std::overflow_error("First vertex exceeds OpenGL GLint range");
		}
		glDrawArrays(_openGLPrimitive(primitive), static_cast<GLint>(firstVertex), static_cast<GLsizei>(vertexCount));
	}

	void Program::render(Primitive primitive, IndexBuffer::Type indexType, std::size_t firstIndex, std::size_t indexCount) const
	{
		_validateGLCount(indexCount);
		const std::size_t stride = indexType == IndexBuffer::Type::UnsignedByte ? 1 : indexType == IndexBuffer::Type::UnsignedShort ? 2
																																	: 4;
		const auto offset = static_cast<std::uintptr_t>(firstIndex * stride);
		glDrawElements(_openGLPrimitive(primitive), static_cast<GLsizei>(indexCount), _openGLIndexType(indexType), reinterpret_cast<const void *>(offset));
	}

	void Program::renderInstanced(Primitive primitive, IndexBuffer::Type indexType, std::size_t firstIndex, std::size_t indexCount, std::size_t instanceCount) const
	{
		_validateGLCount(indexCount);
		_validateGLCount(instanceCount);
		const std::size_t stride = indexType == IndexBuffer::Type::UnsignedByte ? 1 : indexType == IndexBuffer::Type::UnsignedShort ? 2
																																	: 4;
		const auto offset = static_cast<std::uintptr_t>(firstIndex * stride);
		glDrawElementsInstanced(_openGLPrimitive(primitive), static_cast<GLsizei>(indexCount), _openGLIndexType(indexType), reinterpret_cast<const void *>(offset), static_cast<GLsizei>(instanceCount));
	}

	void Program::_applyUniformBlockBindings(GLuint identifier) const
	{
		GLint maximumBindings = 0;
		glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maximumBindings);

		for (const auto &[name, bindingPoint] : _uniformBlockBindings)
		{
			if (bindingPoint >= static_cast<std::size_t>(maximumBindings))
			{
				throw std::out_of_range("Uniform block binding point exceeds OpenGL limit");
			}

			const GLuint blockIndex = glGetUniformBlockIndex(identifier, name.c_str());
			if (blockIndex == GL_INVALID_INDEX)
			{
				throw std::runtime_error("Uniform block not found in program: " + name);
			}

			glUniformBlockBinding(identifier, blockIndex, static_cast<GLuint>(bindingPoint));
		}
	}

	void Program::bindUniformBlock(std::string name, std::size_t bindingPoint)
	{
		if (name.empty())
		{
			throw std::invalid_argument("Uniform block name cannot be empty");
		}

		auto iterator = _uniformBlockBindings.find(name);
		if (iterator != _uniformBlockBindings.end() && iterator->second == bindingPoint)
		{
			return;
		}

		_uniformBlockBindings[std::move(name)] = bindingPoint;
		validate();
	}

	void Program::_applyShaderStorageBlockBindings(GLuint identifier) const
	{
		GLint maximumBindings = 0;
		glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &maximumBindings);

		for (const auto &[name, bindingPoint] : _shaderStorageBlockBindings)
		{
			if (bindingPoint >= static_cast<std::size_t>(maximumBindings))
			{
				throw std::out_of_range("Shader storage block binding point exceeds OpenGL limit");
			}

			const GLuint blockIndex = glGetProgramResourceIndex(identifier, GL_SHADER_STORAGE_BLOCK, name.c_str());
			if (blockIndex == GL_INVALID_INDEX)
			{
				throw std::runtime_error("Shader storage block not found in program: " + name);
			}

			glShaderStorageBlockBinding(identifier, blockIndex, static_cast<GLuint>(bindingPoint));
		}
	}

	void Program::bindShaderStorageBlock(std::string name, std::size_t bindingPoint)
	{
		if (name.empty())
		{
			throw std::invalid_argument("Shader storage block name cannot be empty");
		}

		auto iterator = _shaderStorageBlockBindings.find(name);
		if (iterator != _shaderStorageBlockBindings.end() && iterator->second == bindingPoint)
		{
			return;
		}

		_shaderStorageBlockBindings[std::move(name)] = bindingPoint;
		validate();
	}

	void Program::_applySamplerBindings(GLuint identifier) const
	{
		for (const auto &[name, bindingPoint] : _samplerBindings)
		{
			const GLint location = glGetUniformLocation(identifier, name.c_str());

			if (location == -1)
			{
				continue;
			}

			glProgramUniform1i(identifier, location, static_cast<GLint>(bindingPoint));
		}
	}

	void Program::bindSampler(std::string name, std::size_t bindingPoint)
	{
		_samplerBindings[std::move(name)] = bindingPoint;
		validate();
	}
}
