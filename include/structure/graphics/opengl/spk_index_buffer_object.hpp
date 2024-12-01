#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include <vector>
#include <span>
#include <stdexcept>
#include "spk_vertex_buffer_object.hpp"

namespace spk::OpenGL
{
	class IndexBufferObject : public VertexBufferObject
	{
	public:
		IndexBufferObject();
		void append(const unsigned int& data);
		void append(const std::vector<unsigned int>& data);
		void append(const std::span<unsigned int>& data);

		IndexBufferObject& operator<<(const unsigned int& data);
		IndexBufferObject& operator<<(const std::vector<unsigned int>& data);
		IndexBufferObject& operator<<(const std::span<unsigned int>& data);

		size_t nbIndexes() const;
		size_t nbTriangles() const;
	};
}
