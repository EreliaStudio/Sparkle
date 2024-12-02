#pragma once

#include <GL/glew.h>
#include <GL/gl.h>

namespace spk::OpenGL
{
	/**
	 * @class VertexArrayObject
	 * @brief Manages an OpenGL Vertex Array Object (VAO).
	 * 
	 * The `VertexArrayObject` class encapsulates the creation, management, and cleanup of a Vertex Array Object (VAO) in OpenGL.
	 * It provides methods for activating and deactivating the VAO, ensuring that the necessary OpenGL context is present during operations.
	 * 
	 * Example usage:
	 * @code
	 * spk::OpenGL::VertexArrayObject vao;
	 * vao.activate();
	 * // Set up attributes and buffers...
	 * vao.deactivate();
	 * @endcode
	 */
	class VertexArrayObject
	{
	private:
		GLuint _id; /**< @brief The OpenGL ID of the Vertex Array Object. */

		/**
		 * @brief Allocates the VAO in the current OpenGL context.
		 * 
		 * This method ensures the VAO is created only when a valid OpenGL context is active.
		 */
		void _allocate();

		/**
		 * @brief Releases the VAO from the current OpenGL context.
		 * 
		 * This method deletes the VAO if it exists and resets the internal ID.
		 */
		void _release();

	public:
		/**
		 * @brief Default constructor.
		 * 
		 * Initializes the VAO with no allocated OpenGL ID.
		 */
		VertexArrayObject();

		/**
		 * @brief Destructor.
		 * 
		 * Ensures the VAO is released when the object goes out of scope.
		 */
		~VertexArrayObject();

		/**
		 * @brief Activates the VAO.
		 * 
		 * Binds the VAO for use in the current OpenGL context. If the VAO has not been allocated, it will be created.
		 */
		void activate();

		/**
		 * @brief Deactivates the VAO.
		 * 
		 * Unbinds the currently active VAO.
		 */
		void deactivate();
	};
}
