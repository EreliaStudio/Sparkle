#include "geometry/texture_mesh_2d.hpp"
#include <limits>
#include <stdexcept>
#include <utility>

namespace spk
{
	void TextureMesh2D::Builder::_setupAttributes(VertexBuffer &vertexBuffer)
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
			2);
	}

	void TextureMesh2D::Builder::_setupMesh(TextureMesh2D&)
	{
		
	}
}
