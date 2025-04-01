#pragma once

#include <GL/glew.h>

#include <GL/gl.h>

#include "spk_vertex_buffer_object.hpp"
#include <initializer_list>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace spk::OpenGL
{
	class IndexBufferObject : public VertexBufferObject
	{
	public:
		IndexBufferObject();

		void append(const unsigned int &data);
		void append(std::span<const unsigned int> data);

		IndexBufferObject &operator<<(const unsigned int &data);

		template <typename Container>
			requires requires(const Container &c) {
				{ c.data() };
				{ c.size() };
			}
		IndexBufferObject &operator<<(const Container &container)
		{
			append(std::span<const unsigned int>(container.data(), container.size()));
			return *this;
		}

		template <size_t N>
		IndexBufferObject &operator<<(unsigned int ilist[N])
		{
			append(std::span<const unsigned int>(&ilist, N));
			return *this;
		}

		size_t nbIndexes() const;
		size_t nbTriangles() const;

		std::vector<unsigned int> get();
	};
}
