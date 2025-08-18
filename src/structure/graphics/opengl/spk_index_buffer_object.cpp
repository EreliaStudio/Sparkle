#include "structure/graphics/opengl/spk_index_buffer_object.hpp"

namespace spk::OpenGL
{
	IndexBufferObject::IndexBufferObject() :
		VertexBufferObject(Type::Layout, Usage::Static)
	{
	}

	void IndexBufferObject::append(const unsigned int &p_data)
	{
		VertexBufferObject::append(&p_data, sizeof(unsigned int));
	}

	void IndexBufferObject::append(std::span<const unsigned int> p_data)
	{
		if (!p_data.empty())
		{
			VertexBufferObject::append(p_data.data(), p_data.size() * sizeof(unsigned int));
		}
	}

	IndexBufferObject &IndexBufferObject::operator<<(const unsigned int &p_data)
	{
		append(p_data);
		return *this;
	}

	size_t IndexBufferObject::nbIndexes() const
	{
		return size() / sizeof(unsigned int);
	}

	size_t IndexBufferObject::nbTriangles() const
	{
		return nbIndexes() / 3;
	}

	std::vector<unsigned int> IndexBufferObject::get()
	{
		activate();

		size_t totalSize = this->size();
		if (totalSize == 0)
		{
			return {};
		}

		size_t elementCount = totalSize / sizeof(unsigned int);
		std::vector<unsigned int> result(elementCount);

		glGetBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(totalSize), result.data());

		return result;
	}
}