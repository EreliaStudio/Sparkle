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

		void append(const unsigned int &p_data);
		void append(std::span<const unsigned int> p_data);

		IndexBufferObject &operator<<(const unsigned int &p_data);

		template <typename Container>
			requires requires(const Container &p_c) {
				{ p_c.data() };
				{ p_c.size() };
			}
		IndexBufferObject &operator<<(const Container &p_container)
		{
			append(std::span<const unsigned int>(p_container.data(), p_container.size()));
			return *this;
		}

		template <size_t N>
		IndexBufferObject &operator<<(std::array<unsigned int, N> p_ilist)
		{
			append(std::span<const unsigned int>(&p_ilist, N));
			return *this;
		}

		size_t nbIndexes() const;
		size_t nbTriangles() const;

		std::vector<unsigned int> get();
	};
}
