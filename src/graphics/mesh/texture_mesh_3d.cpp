#include "geometry/texture_mesh_3d.hpp"

namespace spk
{
	void TextureMesh3D::Builder::_setupAttributes(VertexBuffer &vertexBuffer)
	{
		vertexBuffer.addAttribute(0, VertexBuffer::Attribute::Type::Float, 3);
		vertexBuffer.addAttribute(1, VertexBuffer::Attribute::Type::Float, 3);
		vertexBuffer.addAttribute(2, VertexBuffer::Attribute::Type::Float, 2);
	}
}
