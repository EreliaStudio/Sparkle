#pragma once

#include <cstdint>
#include <memory>

#include "graphics/opengl/layout_buffer.hpp"
#include "math/vector2.hpp"

namespace spk
{
	class TextureMesh2D
	{
	public:
		struct Vertex
		{
			Vector2 position;
			float depth;
			Vector2 uv;
		};
		using Index = std::uint32_t;

	private:
		struct Content
		{
			LayoutBuffer layout;
		};

	public:
		class Builder
		{
		private:
			std::shared_ptr<Content> _content;
			void _check() const;

		public:
			Builder();
			[[nodiscard]] Index addVertex(const Vertex &vertex);
			[[nodiscard]] Index addVertex(Vector2 position, float depth, Vector2 uv);
			void addIndex(Index index);
			void addTriangle(Index a, Index b, Index c);
			void addShape(const Vertex &topLeft, const Vertex &bottomLeft, const Vertex &bottomRight, const Vertex &topRight);
			void reserve(std::size_t vertexCount, std::size_t indexCount);
			[[nodiscard]] TextureMesh2D build() &&;
		};

	private:
		explicit TextureMesh2D(std::shared_ptr<const Content> content);

		std::shared_ptr<const Content> _content;
		friend class Builder;

	public:
		TextureMesh2D() = default;
		[[nodiscard]] const LayoutBuffer &layout() const;
		[[nodiscard]] std::size_t vertexCount() const noexcept;
		[[nodiscard]] std::size_t indexCount() const noexcept;
		[[nodiscard]] IndexBuffer::Type indexType() const noexcept;
		[[nodiscard]] bool empty() const noexcept;
	};

}
