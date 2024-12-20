#pragma once

#include <GL/glew.h>
#include <string>
#include <stdexcept>

namespace spk::OpenGL
{
	class Program
	{
	private:
		std::string _vertexShaderCode;
		std::string _fragmentShaderCode;

		GLuint _programID;

		GLuint _compileShader(const std::string& shaderCode, GLenum shaderType);
		GLuint _linkProgram(GLuint vertexShader, GLuint fragmentShader);
		void _load();

	public:
		Program();
		Program(const std::string& p_vertexShaderCode, const std::string& p_fragmentShaderCode);

		void activate();
		void deactivate();
		void render(GLsizei nbIndexes, GLsizei p_nbInstance);
		void validate();

		~Program();
	};
}
