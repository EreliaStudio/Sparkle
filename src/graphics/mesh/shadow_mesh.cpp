#include "geometry/shadow_mesh.hpp"

namespace spk::ShadowMap
{
	void Mesh::Builder::_setupAttributes(spk::VertexBuffer &buffer)
	{
		buffer.addAttribute(0, spk::VertexBuffer::Attribute::Type::Float, 3);
	}
}
