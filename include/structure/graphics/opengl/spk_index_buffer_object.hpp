#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include <vector>
#include <span>
#include <stdexcept>
#include "spk_vertex_buffer_object.hpp"

namespace spk::OpenGL
{
	/**
	 * @class IndexBufferObject
	 * @brief Manages an OpenGL Index Buffer Object (IBO) for rendering indexed geometry.
	 * 
	 * The IndexBufferObject allows managing index data for rendering triangles using indexed geometry.
	 * 
	 * Example usage:
	 * @code
	 * spk::OpenGL::IndexBufferObject ibo;
	 * 
	 * std::vector<unsigned int> indices = {0, 1, 2, 2, 1, 3};
	 * ibo << indices; // Upload indices to the buffer
	 * ibo.validate();
	 * 
	 * size_t triangles = ibo.nbTriangles(); // Get the number of triangles
	 * @endcode
	 */
	class IndexBufferObject : public VertexBufferObject
	{
	public:
		/**
		 * @brief Constructs an empty IndexBufferObject.
		 */
		IndexBufferObject();

		/**
		 * @brief Appends a single index to the buffer.
		 * @param data The index to append.
		 */
		void append(const unsigned int& data);

		/**
		 * @brief Appends a vector of indices to the buffer.
		 * @param data The vector of indices to append.
		 */
		void append(const std::vector<unsigned int>& data);

		/**
		 * @brief Appends a span of indices to the buffer.
		 * @param data The span of indices to append.
		 */
		void append(const std::span<unsigned int>& data);

		/**
		 * @brief Overload of the insertion operator to append a single index.
		 * @param data The index to append.
		 * @return A reference to this object.
		 */
		IndexBufferObject& operator<<(const unsigned int& data);

		/**
		 * @brief Overload of the insertion operator to append a vector of indices.
		 * @param data The vector of indices to append.
		 * @return A reference to this object.
		 */
		IndexBufferObject& operator<<(const std::vector<unsigned int>& data);

		/**
		 * @brief Overload of the insertion operator to append a span of indices.
		 * @param data The span of indices to append.
		 * @return A reference to this object.
		 */
		IndexBufferObject& operator<<(const std::span<unsigned int>& data);

		/**
		 * @brief Gets the number of indices stored in the buffer.
		 * @return The number of indices.
		 */
		size_t nbIndexes() const;

		/**
		 * @brief Computes the number of triangles based on the indices stored.
		 * Assumes the buffer represents a triangle list where each triangle uses three indices.
		 * @return The number of triangles.
		 */
		size_t nbTriangles() const;
	};
}