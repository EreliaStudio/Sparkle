#include "texture_mesh_2d.hpp"
#include <limits>
#include <stdexcept>
#include <utility>

namespace spk
{
	TextureMesh2D::TextureMesh2D(std::shared_ptr<const Content> c) :
		_content(std::move(c))
	{
	}
	const LayoutBuffer &TextureMesh2D::layout() const
	{
		if (!_content)
		{
			throw std::logic_error("TextureMesh2D is empty");
		}
		return _content->layout;
	}
	std::size_t TextureMesh2D::vertexCount() const noexcept
	{
		return _content ? _content->layout.vertexBuffer().count() : 0;
	}
	std::size_t TextureMesh2D::indexCount() const noexcept
	{
		return _content ? _content->layout.indexBuffer().count() : 0;
	}
	IndexBuffer::Type TextureMesh2D::indexType() const noexcept
	{
		return IndexBuffer::Type::UnsignedInt;
	}
	bool TextureMesh2D::empty() const noexcept
	{
		return indexCount() == 0;
	}
	void TextureMesh2D::Builder::_check() const
	{
		if (!_content)
		{
			throw std::logic_error("TextureMesh2D::Builder was already consumed");
		}
	}
	TextureMesh2D::Builder::Builder() :
		_content(std::make_shared<Content>())
	{
		auto &v = _content->layout.vertexBuffer();
		v.addAttribute(0, VertexBuffer::Attribute::Type::Float, 2);
		v.addAttribute(1, VertexBuffer::Attribute::Type::Float, 1);
		v.addAttribute(2, VertexBuffer::Attribute::Type::Float, 2);
		_content->layout.indexBuffer().setType(IndexBuffer::Type::UnsignedInt);
	}
	TextureMesh2D::Index TextureMesh2D::Builder::addVertex(const Vertex &v)
	{
		_check();
		auto n = _content->layout.vertexBuffer().count();
		if (n > std::numeric_limits<Index>::max())
		{
			throw std::overflow_error("TextureMesh2D vertex index overflow");
		}
		_content->layout.vertexBuffer().pushBack(v);
		return static_cast<Index>(n);
	}
	TextureMesh2D::Index TextureMesh2D::Builder::addVertex(Vector2 p, float d, Vector2 uv)
	{
		return addVertex({p, d, uv});
	}
	void TextureMesh2D::Builder::addIndex(Index i)
	{
		_check();
		if (i >= _content->layout.vertexBuffer().count())
		{
			throw std::out_of_range("TextureMesh2D index references a missing vertex");
		}
		_content->layout.indexBuffer().pushBack(i);
	}
	void TextureMesh2D::Builder::addTriangle(Index a, Index b, Index c)
	{
		addIndex(a);
		addIndex(b);
		addIndex(c);
	}
	void TextureMesh2D::Builder::addShape(const Vertex &a, const Vertex &b, const Vertex &c, const Vertex &d)
	{
		auto i = addVertex(a);
		(void)addVertex(b);
		(void)addVertex(c);
		(void)addVertex(d);
		addTriangle(i, i + 1, i + 2);
		addTriangle(i, i + 2, i + 3);
	}
	void TextureMesh2D::Builder::reserve(std::size_t vertexCount, std::size_t indexCount)
	{
		_check();
		_content->layout.vertexBuffer().reserve<Vertex>(vertexCount);
		_content->layout.indexBuffer().reserve<Index>(indexCount);
	}
	TextureMesh2D TextureMesh2D::Builder::build() &&
	{
		_check();
		_content->layout.validate();
		return TextureMesh2D(std::exchange(_content, nullptr));
	}
}
