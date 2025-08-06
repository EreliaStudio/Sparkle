#pragma once

#include <GL/glew.h>

#include <GL/gl.h>

#include <stdexcept>
#include <string>

namespace spk::OpenGL
{
	class Program
	{
	private:
		std::string _vertexShaderCode;
		std::string _fragmentShaderCode;

		bool _verboseMode = false;
		bool _needCleanup = false;
		GLuint _programID;

		GLuint _compileShader(const std::string &shaderCode, GLenum shaderType);
		GLuint _linkProgram(GLuint vertexShader, GLuint fragmentShader);
		void _load();
		void _cleanup();

	public:
		Program();
		Program(const std::string &p_vertexShaderCode, const std::string &p_fragmentShaderCode, bool p_verboseMode = false);

		void load(const std::string &p_vertexShaderCode, const std::string &p_fragmentShaderCode);

		void setVerboseMode(bool p_verbose = true);

		void activate();
		void deactivate();
		void render(GLsizei nbIndexes, GLsizei p_nbInstance);
		void validate();

		~Program();
	};
}
