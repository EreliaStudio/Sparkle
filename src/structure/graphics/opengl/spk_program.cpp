#include "structure/graphics/opengl/spk_program.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <Windows.h>

#include <vector>

#include "structure/spk_iostream.hpp"

namespace spk::OpenGL
{
	Program::Program() :
		_vertexShaderCode(""),
		_fragmentShaderCode(""),
		_programID(0)
	{
	}

	Program::Program(const std::string &p_vertexShaderCode, const std::string &p_fragmentShaderCode) :
		_vertexShaderCode(p_vertexShaderCode),
		_fragmentShaderCode(p_fragmentShaderCode),
		_programID(0)
	{
	}

	void Program::load(const std::string &p_vertexShaderCode, const std::string &p_fragmentShaderCode)
	{
		_vertexShaderCode = p_vertexShaderCode;
		_fragmentShaderCode = p_fragmentShaderCode;
		_needCleanup = true;
	}

	GLuint Program::_compileShader(const std::string &p_shaderCode, GLenum p_shaderType)
	{
		GLuint shader = glCreateShader(p_shaderType);
		const char *source = p_shaderCode.c_str();
		glShaderSource(shader, 1, &source, nullptr);
		glCompileShader(shader);

		GLint success;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			GLchar infoLog[512];
			glGetShaderInfoLog(shader, 512, nullptr, infoLog);
			std::string shaderTypeStr = (p_shaderType == GL_VERTEX_SHADER) ? "Vertex Shader" : "Fragment Shader";
			throw std::runtime_error(shaderTypeStr + " compilation failed: " + std::string(infoLog));
		}

		return shader;
	}

	GLuint Program::_linkProgram(GLuint p_vertexShader, GLuint p_fragmentShader)
	{
		GLuint program = glCreateProgram();
		glAttachShader(program, p_vertexShader);
		glAttachShader(program, p_fragmentShader);
		glLinkProgram(program);

		GLint success;
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success)
		{
			GLchar infoLog[512];
			glGetProgramInfoLog(program, 512, nullptr, infoLog);
			throw std::runtime_error("Shader Program linking failed: " + std::string(infoLog));
		}

		glDeleteShader(p_vertexShader);
		glDeleteShader(p_fragmentShader);

		return program;
	}

	void Program::_load()
	{
		GLuint vertexShader = _compileShader(_vertexShaderCode, GL_VERTEX_SHADER);
		GLuint fragmentShader = _compileShader(_fragmentShaderCode, GL_FRAGMENT_SHADER);
		_programID = _linkProgram(vertexShader, fragmentShader);
	}

	void Program::_cleanup()
	{
		if (wglGetCurrentContext() != nullptr && _programID != 0)
		{
			glDeleteProgram(_programID);
		}
		_programID = 0;
		_needCleanup = false;
	}

	void Program::activate()
	{
		if (_needCleanup == true)
		{
			_cleanup();
		}

		if (_programID == 0)
		{
			_load();
		}

		glUseProgram(_programID);
	}

	void Program::deactivate()
	{
		if (_programID != 0)
		{
			glUseProgram(0);
		}
	}

	void Program::render(GLsizei p_nbIndexes, GLsizei p_nbInstance)
	{
		
		if (_programID == 0)
		{
			_load();
		}

		if (p_nbInstance == 0)
		{
			glDrawElements(GL_TRIANGLES, p_nbIndexes, GL_UNSIGNED_INT, nullptr);
		}
		else
		{
			glDrawElementsInstanced(GL_TRIANGLES, p_nbIndexes, GL_UNSIGNED_INT, nullptr, p_nbInstance);
		}
	}

	void Program::validate()
	{
		glValidateProgram(_programID);
		GLint validationStatus;
		glGetProgramiv(_programID, GL_VALIDATE_STATUS, &validationStatus);
		if (validationStatus == GL_FALSE)
		{
			GLint infoLogLength;
			glGetProgramiv(_programID, GL_INFO_LOG_LENGTH, &infoLogLength);
			std::vector<char> infoLog(infoLogLength);
			glGetProgramInfoLog(_programID, infoLogLength, &infoLogLength, infoLog.data());
			throw std::runtime_error("Shader Program validation failed: " + std::string(infoLog.data()));
		}
	}

	Program::~Program()
	{
		_cleanup();
	}
}
