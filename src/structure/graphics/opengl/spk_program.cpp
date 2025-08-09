#include "structure/graphics/opengl/spk_program.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <Windows.h>

#include <vector>

#include "structure/spk_iostream.hpp"

#include "structure/system/spk_exception.hpp"

#include "utils/spk_opengl_utils.hpp"

namespace spk::OpenGL
{
	Program::Program() :
		_vertexShaderCode(""),
		_fragmentShaderCode(""),
		_programID(0)
	{
	}

	Program::Program(const std::string &p_vertexShaderCode, const std::string &p_fragmentShaderCode, bool p_verboseMode) :
		_vertexShaderCode(p_vertexShaderCode),
		_fragmentShaderCode(p_fragmentShaderCode),
		_programID(0),
		_verboseMode(p_verboseMode)
	{
	}

	void Program::load(const std::string &p_vertexShaderCode, const std::string &p_fragmentShaderCode)
	{
		_vertexShaderCode = p_vertexShaderCode;
		_fragmentShaderCode = p_fragmentShaderCode;
		_needCleanup = true;
	}

	void Program::setVerboseMode(bool p_verbose)
	{
		_verboseMode = p_verbose;
	}

	GLuint Program::_compileShader(const std::string &p_shaderCode, GLenum p_shaderType)
	{
		GLuint shader = glCreateShader(p_shaderType);
		const char *source = p_shaderCode.c_str();
		glShaderSource(shader, 1, &source, nullptr);
		glCompileShader(shader);

		// Always print the compile log

		if (_verboseMode == true)
		{
			OpenGLUtils::PrintShaderLog(shader, (p_shaderType == GL_VERTEX_SHADER) ? "Vertex" : "Fragment");
		}

		GLint success = GL_FALSE;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{

			if (_verboseMode == true)
			{
				OpenGLUtils::LogGLErrors("_compileShader");
			}
			std::string shaderTypeStr = (p_shaderType == GL_VERTEX_SHADER) ? "Vertex Shader" : "Fragment Shader";
			GLint len = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
			std::vector<char> infoLog((size_t)std::max(1, len));
			if (len > 1)
			{
				glGetShaderInfoLog(shader, len, nullptr, infoLog.data());
			}
			GENERATE_ERROR(shaderTypeStr + std::string(" compilation failed: ") + (len > 1 ? infoLog.data() : "(no log)"));
		}

		return shader;
	}

	GLuint Program::_linkProgram(GLuint p_vertexShader, GLuint p_fragmentShader)
	{
		GLuint program = glCreateProgram();
		glAttachShader(program, p_vertexShader);
		glAttachShader(program, p_fragmentShader);
		glLinkProgram(program);

		// Always print the link log

		if (_verboseMode == true)
		{
			OpenGLUtils::PrintProgramLog(program, "Link");
		}

		GLint success = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success)
		{

			if (_verboseMode == true)
			{
				OpenGLUtils::LogGLErrors("_linkProgram");
			}
			GLint len = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
			std::vector<char> infoLog((size_t)std::max(1, len));
			if (len > 1)
			{
				glGetProgramInfoLog(program, len, nullptr, infoLog.data());
			}
			GENERATE_ERROR(std::string("Shader Program linking failed: ") + (len > 1 ? infoLog.data() : "(no log)"));
		}

		// Reflect program to help debugging bindings/locations

		if (_verboseMode == true)
		{
			OpenGLUtils::DumpActiveAttribs(program);
		}

		if (_verboseMode == true)
		{
			OpenGLUtils::DumpActiveUniformsAndSamplers(program);
		}

		if (_verboseMode == true)
		{
			OpenGLUtils::DumpUniformBlocks(program);
		}

		// Clean up shaders after link
		glDeleteShader(p_vertexShader);
		glDeleteShader(p_fragmentShader);

		return program;
	}

	void Program::_load()
	{
		// Context sanity info
		if (wglGetCurrentContext() == nullptr)
		{
			GENERATE_ERROR("No current OpenGL context before creating program");
		}
		if (_verboseMode == true)
		{
		std::cerr << "[GL] Version: " << (const char *)glGetString(GL_VERSION) << " | Renderer: " << (const char *)glGetString(GL_RENDERER)
				  << " | GLSL: " << (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
		}

		GLuint vertexShader = _compileShader(_vertexShaderCode, GL_VERTEX_SHADER);
		GLuint fragmentShader = _compileShader(_fragmentShaderCode, GL_FRAGMENT_SHADER);
		_programID = _linkProgram(vertexShader, fragmentShader);

		if (_verboseMode == true)
		{
			OpenGLUtils::LogGLErrors("Program::_load");
		}
	}

	void Program::_cleanup()
	{
		if (wglGetCurrentContext() != nullptr && _programID != 0)
		{
			glDeleteProgram(_programID);

			if (_verboseMode == true)
			{
				OpenGLUtils::LogGLErrors("Program::_cleanup glDeleteProgram");
			}
		}
		_programID = 0;
		_needCleanup = false;
	}

	void Program::activate()
	{
		// Lazy recompile if requested via load()
		if (_needCleanup == true)
		{
			_cleanup();
		}

		if (_programID == 0)
		{
			_load();
		}

		glUseProgram(_programID);

		if (_verboseMode == true)
		{
			OpenGLUtils::LogGLErrors("Program::activate glUseProgram");
		}
	}

	void Program::deactivate()
	{
		if (_programID != 0)
		{
			glUseProgram(0);

			if (_verboseMode == true)
			{
				OpenGLUtils::LogGLErrors("Program::deactivate glUseProgram(0)");
			}
		}
	}

	void Program::render(GLsizei p_nbIndexes, GLsizei p_nbInstance)
	{
		if (_programID == 0)
		{
			_load();
		}

		if (p_nbInstance == 0 || p_nbIndexes == 0)
		{
			return;
		}

		if (_verboseMode == true)
		{
			OpenGLUtils::DumpPreDrawState();
		}

#ifndef NDEBUG
		try
		{
			validate();
		} catch (...)
		{
			throw;
		}
#endif

		GLint ebo = 0;
		glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &ebo);
		if (ebo == 0)
		{
			std::cerr << "!! No element array buffer bound in current VAO; glDrawElements* will fail in core profile.\n";
		}

		if (_verboseMode == true)
		{
			OpenGLUtils::DumpUBOBindingsForUsedBlocks(_programID);
		}

		if (_verboseMode == true)
		{
			OpenGLUtils::DumpTexture2DCompletenessForUnit(0);
		}

		if (_verboseMode == true)
		{
			OpenGLUtils::CheckIndexBufferCapacityVsCount(p_nbIndexes, GL_UNSIGNED_INT);
		}

		glDrawElementsInstanced(GL_TRIANGLES, p_nbIndexes, GL_UNSIGNED_INT, nullptr, p_nbInstance);

		if (_verboseMode == true)
		{
			OpenGLUtils::LogGLErrors("Program::render glDrawElementsInstanced");
		}
	}

	void Program::validate()
	{
		if (_programID == 0)
		{
			GENERATE_ERROR("Program::validate called with no program");
		}

		glValidateProgram(_programID);
		GLint validationStatus = GL_FALSE;
		glGetProgramiv(_programID, GL_VALIDATE_STATUS, &validationStatus);
		GLint infoLogLength = 0;
		glGetProgramiv(_programID, GL_INFO_LOG_LENGTH, &infoLogLength);
		std::vector<char> infoLog((size_t)std::max(1, infoLogLength));
		if (infoLogLength > 1)
		{
			glGetProgramInfoLog(_programID, infoLogLength, &infoLogLength, infoLog.data());
		}

		if (_verboseMode == true)
		{
		std::cerr << "[Program] Validate status=" << (validationStatus ? "OK" : "FAIL") << " log:\n"
				  << (infoLogLength > 1 ? infoLog.data() : "(no log)") << "\n";
		}

		if (validationStatus == GL_FALSE)
		{
			// If validation fails, also dump pre-draw state to help pinpoint
			if (_verboseMode == true)
			{
				OpenGLUtils::DumpPreDrawState();
			}
			GENERATE_ERROR(std::string("Shader Program validation failed: ") + (infoLogLength > 1 ? infoLog.data() : "(no log)"));
		}
	}

	Program::~Program()
	{
		_cleanup();
	}
}
