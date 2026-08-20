#include "layout_buffer.hpp"

namespace spk
{
	LayoutBuffer::LayoutBuffer()
	{
		_vertexArray.setVertexBuffer(_vertexBuffer);
		_vertexArray.setIndexBuffer(_indexBuffer);
	}

	void LayoutBuffer::activate(RenderContext &context) const
	{
		_vertexArray.activate(context);
	}

	VertexBuffer &LayoutBuffer::vertexBuffer() noexcept
	{
		return _vertexBuffer;
	}

	const VertexBuffer &LayoutBuffer::vertexBuffer() const noexcept
	{
		return _vertexBuffer;
	}

	IndexBuffer &LayoutBuffer::indexBuffer() noexcept
	{
		return _indexBuffer;
	}

	const IndexBuffer &LayoutBuffer::indexBuffer() const noexcept
	{
		return _indexBuffer;
	}
}
