#include "graphics/pipeline/spk_pipeline.hpp"
#include <regex>
namespace spk
{
	std::string addLineNumbers(const std::string& p_shaderCode)
	{
		std::istringstream stream(p_shaderCode);
		std::ostringstream result;
		std::string line;
		int lineNumber = 1;

		while (std::getline(stream, line)) {
			result << "[" << std::setw(4) << lineNumber << "] " << line << '\n';
			lineNumber++;
		}

		return result.str();
	}

	GLuint compileShaderModule(int VertexEnumType, const std::string& p_shaderCode)
	{
		GLuint result = glCreateShader(VertexEnumType);
		GLint errorValue;

		const char* code = p_shaderCode.c_str();

		errorValue = GL_FALSE;

		glShaderSource(result, 1, &(code), NULL);
		glCompileShader(result);
		glGetShaderiv(result, GL_COMPILE_STATUS, &errorValue);

		if (errorValue != GL_TRUE)
		{
			GLint len;

			glGetShaderiv(result, GL_INFO_LOG_LENGTH, &len);
			char *errorMsg = new char[len + 1];
			glGetShaderInfoLog(result, len, NULL, errorMsg);
			throwException("Error while compiling shader\nShader code :\n" + addLineNumbers(p_shaderCode) + "\nError : " + errorMsg);
		}

		return (result);
	}

	bool compileProgram(GLuint presultID, GLuint p_vertexID, GLuint p_fragmentID)
	{
		GLint result;

		result = GL_FALSE;
		glAttachShader(presultID, p_vertexID);
		glAttachShader(presultID, p_fragmentID);
		glLinkProgram(presultID);
		glGetProgramiv(presultID, GL_LINK_STATUS, &result);
		if (result != GL_TRUE)
		{
			return (false);
		}
		glDetachShader(presultID, p_vertexID);
		glDetachShader(presultID, p_fragmentID);
		return (true);
	}
	
	GLuint loadProgram(const std::string& p_vertexCode, const std::string& p_fragmentCode)
	{
		GLuint result = glCreateProgram();

		GLuint vertexShaderModule = compileShaderModule(GL_VERTEX_SHADER, p_vertexCode);
		GLuint fragmentShaderModule = compileShaderModule(GL_FRAGMENT_SHADER, p_fragmentCode);

		if (compileProgram(result, vertexShaderModule, fragmentShaderModule) == false)
		{
			throwException("Error while linking program.\n\n ------------\nVertex Shader :\n" + addLineNumbers(p_vertexCode) + "\n\n ------------\nFragment Shader :\n" + addLineNumbers(p_fragmentCode));
		}

		glDeleteShader(vertexShaderModule);
		glDeleteShader(fragmentShaderModule);

		return (result);
	}
}