#pragma once

#include "index_buffer.hpp"
#include "vertex_array.hpp"
#include "vertex_buffer.hpp"

namespace spk
{
	class RenderContext;

	class LayoutBuffer
	{
	private:
		VertexBuffer _vertexBuffer;
		IndexBuffer _indexBuffer;
		VertexArray _vertexArray;

	public:
		LayoutBuffer();
		LayoutBuffer(const LayoutBuffer &) = delete;
		LayoutBuffer(LayoutBuffer &&) = delete;
		~LayoutBuffer() = default;

		LayoutBuffer &operator=(const LayoutBuffer &) = delete;
		LayoutBuffer &operator=(LayoutBuffer &&) = delete;

		void activate(RenderContext &context) const;
		[[nodiscard]] VertexBuffer &vertexBuffer() noexcept;
		[[nodiscard]] const VertexBuffer &vertexBuffer() const noexcept;
		[[nodiscard]] IndexBuffer &indexBuffer() noexcept;
		[[nodiscard]] const IndexBuffer &indexBuffer() const noexcept;
	};
}
