#include "geometry/color_mesh_2d.hpp"

namespace spk
{
	void ColorMesh2D::Builder::_setupAttributes(VertexLayout &vertexBuffer)
	{
		vertexBuffer.addAttribute(0, VertexLayout::Attribute::Type::Float, 2);
		vertexBuffer.addAttribute(1, VertexLayout::Attribute::Type::Float, 1);
		vertexBuffer.addAttribute(2, VertexLayout::Attribute::Type::Float, 4);
	}
}
