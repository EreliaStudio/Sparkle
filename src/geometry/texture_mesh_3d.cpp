#include "geometry/texture_mesh_3d.hpp"

namespace spk
{
	void TextureMesh3D::Builder::_setupAttributes(VertexLayout &vertexBuffer)
	{
		vertexBuffer.addAttribute(0, VertexLayout::Attribute::Type::Float, 3);
		vertexBuffer.addAttribute(1, VertexLayout::Attribute::Type::Float, 3);
		vertexBuffer.addAttribute(2, VertexLayout::Attribute::Type::Float, 2);
	}
}
