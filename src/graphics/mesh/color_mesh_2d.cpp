#include "geometry/color_mesh_2d.hpp"

namespace spk
{
	void ColorMesh2D::Builder::_setupAttributes(VertexBuffer &vertexBuffer)
	{
		vertexBuffer.addAttribute(
			0,
			VertexBuffer::Attribute::Type::Float,
			2);

		vertexBuffer.addAttribute(
			1,
			VertexBuffer::Attribute::Type::Float,
			1);

		vertexBuffer.addAttribute(
			2,
			VertexBuffer::Attribute::Type::Float,
			4);
	}

	void ColorMesh2D::Builder::_setupMesh(ColorMesh2D&)
	{
		
	}
}
