#include "structure/graphics/opengl/spk_index_buffer_object.hpp"

namespace spk::OpenGL
{
	IndexBufferObject::IndexBufferObject() :
		VertexBufferObject(Type::Layout, Usage::Static)
	{
	}

	void IndexBufferObject::append(const unsigned int& data)
	{
		VertexBufferObject::append(&data, sizeof(unsigned int));
	}

	void IndexBufferObject::append(std::span<const unsigned int> data)
	{
		if (!data.empty())
			VertexBufferObject::append(data.data(), data.size() * sizeof(unsigned int));
	}

	IndexBufferObject& IndexBufferObject::operator<<(const unsigned int& data)
	{
		append(data);
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
}