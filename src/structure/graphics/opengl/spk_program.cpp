#include "structure/graphics/opengl/spk_program.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

namespace spk::OpenGL
{
	Program::Program(const std::string& p_vertexShaderCode, const std::string& p_fragmentShaderCode) :
		_vertexShaderCode(p_vertexShaderCode),
		_fragmentShaderCode(p_fragmentShaderCode),
		_programID(0)
	{
	}

	GLuint Program::_compileShader(const std::string& shaderCode, GLenum shaderType)
	{
		GLuint shader = glCreateShader(shaderType);
		const char* source = shaderCode.c_str();
		glShaderSource(shader, 1, &source, nullptr);
		glCompileShader(shader);

		GLint success;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			GLchar infoLog[512];
			glGetShaderInfoLog(shader, 512, nullptr, infoLog);
			std::string shaderTypeStr = (shaderType == GL_VERTEX_SHADER) ? "Vertex Shader" : "Fragment Shader";
			throw std::runtime_error(shaderTypeStr + " compilation failed: " + std::string(infoLog));
		}

		return shader;
	}

	GLuint Program::_linkProgram(GLuint vertexShader, GLuint fragmentShader)
	{
		GLuint program = glCreateProgram();
		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);
		glLinkProgram(program);

		GLint success;
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success)
		{
			GLchar infoLog[512];
			glGetProgramInfoLog(program, 512, nullptr, infoLog);
			throw std::runtime_error("Shader Program linking failed: " + std::string(infoLog));
		}

		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		return program;
	}

	void Program::_load()
	{
		GLuint vertexShader = _compileShader(_vertexShaderCode, GL_VERTEX_SHADER);
		GLuint fragmentShader = _compileShader(_fragmentShaderCode, GL_FRAGMENT_SHADER);
		_programID = _linkProgram(vertexShader, fragmentShader);
	}

	void Program::activate()
	{
		if (_programID == 0)
			_load();

		glUseProgram(_programID);
	}

	void Program::deactivate()
	{
		if (_programID != 0)
			glUseProgram(0);
	}

	void Program::render(GLsizei nbIndexes, GLsizei p_nbInstance)
	{
		if (_programID == 0)
			_load();

		glDrawElementsInstanced(GL_TRIANGLES, nbIndexes, GL_UNSIGNED_INT, nullptr, p_nbInstance);
	}

	Program::~Program()
	{
		if (wglGetCurrentContext() != nullptr && _programID != 0)
		{
			glDeleteProgram(_programID);
		}
	}
}
