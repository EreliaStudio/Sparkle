#include "geometry/texture_mesh_2d.hpp"
#include <limits>
#include <stdexcept>
#include <utility>

namespace spk
{
	void TextureMesh2D::Builder::_setupAttributes(VertexLayout &vertexBuffer)
	{
		vertexBuffer.addAttribute(0, VertexLayout::Attribute::Type::Float, 2);
		vertexBuffer.addAttribute(1, VertexLayout::Attribute::Type::Float, 1);
		vertexBuffer.addAttribute(2, VertexLayout::Attribute::Type::Float, 2);
	}
}
