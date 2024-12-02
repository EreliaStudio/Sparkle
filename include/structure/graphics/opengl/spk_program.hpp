#pragma once

#include <GL/glew.h>
#include <string>
#include <stdexcept>

namespace spk::OpenGL
{
	/**
	 * @class Program
	 * @brief Manages an OpenGL shader program for rendering.
	 * 
	 * The `Program` class encapsulates the compilation, linking, and usage of vertex and fragment shaders.
	 * It provides an easy-to-use interface for activating, deactivating, and rendering with the program.
	 * 
	 * Example usage:
	 * @code
	 * const std::string vertexShaderCode = R"(
	 *	 #version 330 core
	 *	 layout(location = 0) in vec3 position;
	 *	 void main() {
	 *		 gl_Position = vec4(position, 1.0);
	 *	 }
	 * )";
	 * 
	 * const std::string fragmentShaderCode = R"(
	 *	 #version 330 core
	 *	 out vec4 color;
	 *	 void main() {
	 *		 color = vec4(1.0, 0.0, 0.0, 1.0);
	 *	 }
	 * )";
	 * 
	 * spk::OpenGL::Program program(vertexShaderCode, fragmentShaderCode);
	 * program.activate();
	 * program.render(36, 1); // Render a mesh with 36 indexes and 1 instance
	 * program.deactivate();
	 * @endcode
	 */
	class Program
	{
	private:
		std::string _vertexShaderCode; ///< GLSL code for the vertex shader.
		std::string _fragmentShaderCode; ///< GLSL code for the fragment shader.

		GLuint _programID; ///< OpenGL ID of the compiled and linked program.

		/**
		 * @brief Compiles a shader from the provided GLSL code.
		 * @param shaderCode The GLSL code for the shader.
		 * @param shaderType The type of shader (e.g., GL_VERTEX_SHADER or GL_FRAGMENT_SHADER).
		 * @return The OpenGL ID of the compiled shader.
		 * @throws std::runtime_error If the shader compilation fails.
		 */
		GLuint _compileShader(const std::string& shaderCode, GLenum shaderType);

		/**
		 * @brief Links the vertex and fragment shaders into a program.
		 * @param vertexShader The OpenGL ID of the compiled vertex shader.
		 * @param fragmentShader The OpenGL ID of the compiled fragment shader.
		 * @return The OpenGL ID of the linked program.
		 * @throws std::runtime_error If the program linking fails.
		 */
		GLuint _linkProgram(GLuint vertexShader, GLuint fragmentShader);

		/**
		 * @brief Loads the shader program by compiling and linking the provided shaders.
		 * @throws std::runtime_error If compilation or linking fails.
		 */
		void _load();

	public:
		/**
		 * @brief Constructs a shader program from vertex and fragment shader code.
		 * @param p_vertexShaderCode GLSL code for the vertex shader.
		 * @param p_fragmentShaderCode GLSL code for the fragment shader.
		 */
		Program(const std::string& p_vertexShaderCode, const std::string& p_fragmentShaderCode);

		/**
		 * @brief Activates the shader program for rendering.
		 */
		void activate();

		/**
		 * @brief Deactivates the shader program.
		 */
		void deactivate();

		/**
		 * @brief Renders geometry using the shader program.
		 * @param nbIndexes The number of indexes to render.
		 * @param p_nbInstance The number of instances to render.
		 */
		void render(GLsizei nbIndexes, GLsizei p_nbInstance);

		/**
		 * @brief Destructor. Releases resources associated with the shader program.
		 */
		~Program();
	};
}
