#include "graphics/mesh/color_mesh_2d.hpp"
#include <limits>
#include <stdexcept>
#include <utility>

namespace spk
{
	ColorMesh2D::ColorMesh2D(std::shared_ptr<const Content> c) :
		_content(std::move(c))
	{
	}
	const LayoutBuffer &ColorMesh2D::layout() const
	{
		if (!_content)
		{
			throw std::logic_error("ColorMesh2D is empty");
		}
		return _content->layout;
	}
	std::size_t ColorMesh2D::vertexCount() const noexcept
	{
		return _content ? _content->layout.vertexBuffer().count() : 0;
	}
	std::size_t ColorMesh2D::indexCount() const noexcept
	{
		return _content ? _content->layout.indexBuffer().count() : 0;
	}
	IndexBuffer::Type ColorMesh2D::indexType() const noexcept
	{
		return IndexBuffer::Type::UnsignedInt;
	}
	bool ColorMesh2D::empty() const noexcept
	{
		return indexCount() == 0;
	}
	void ColorMesh2D::Builder::_check() const
	{
		if (!_content)
		{
			throw std::logic_error("ColorMesh2D::Builder was already consumed");
		}
	}
	ColorMesh2D::Builder::Builder() :
		_content(std::make_shared<Content>())
	{
		auto &v = _content->layout.vertexBuffer();
		v.addAttribute(0, VertexBuffer::Attribute::Type::Float, 2);
		v.addAttribute(1, VertexBuffer::Attribute::Type::Float, 1);
		v.addAttribute(2, VertexBuffer::Attribute::Type::Float, 4);
		_content->layout.indexBuffer().setType(IndexBuffer::Type::UnsignedInt);
	}
	ColorMesh2D::Index ColorMesh2D::Builder::addVertex(const Vertex &v)
	{
		_check();
		auto n = _content->layout.vertexBuffer().count();
		if (n > std::numeric_limits<Index>::max())
		{
			throw std::overflow_error("ColorMesh2D vertex index overflow");
		}
		_content->layout.vertexBuffer().pushBack(v);
		return static_cast<Index>(n);
	}
	ColorMesh2D::Index ColorMesh2D::Builder::addVertex(Vector2 p, float d, Color c)
	{
		return addVertex({p, d, c});
	}
	void ColorMesh2D::Builder::addIndex(Index i)
	{
		_check();
		if (i >= _content->layout.vertexBuffer().count())
		{
			throw std::out_of_range("ColorMesh2D index references a missing vertex");
		}
		_content->layout.indexBuffer().pushBack(i);
	}
	void ColorMesh2D::Builder::addTriangle(Index a, Index b, Index c)
	{
		addIndex(a);
		addIndex(b);
		addIndex(c);
	}
	void ColorMesh2D::Builder::addShape(const Vertex &a, const Vertex &b, const Vertex &c, const Vertex &d)
	{
		auto i = addVertex(a);
		(void)addVertex(b);
		(void)addVertex(c);
		(void)addVertex(d);
		addTriangle(i, i + 1, i + 2);
		addTriangle(i, i + 2, i + 3);
	}
	void ColorMesh2D::Builder::reserve(std::size_t vertexCount, std::size_t indexCount)
	{
		_check();
		_content->layout.vertexBuffer().reserve<Vertex>(vertexCount);
		_content->layout.indexBuffer().reserve<Index>(indexCount);
	}
	ColorMesh2D ColorMesh2D::Builder::build() &&
	{
		_check();
		_content->layout.validate();
		return ColorMesh2D(std::exchange(_content, nullptr));
	}
}
