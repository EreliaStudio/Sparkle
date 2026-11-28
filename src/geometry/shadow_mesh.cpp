#include "geometry/shadow_mesh.hpp"

namespace spk::ShadowMap
{
	void Mesh::Builder::_setupAttributes(spk::VertexLayout &buffer)
	{
		buffer.addAttribute(0, spk::VertexLayout::Attribute::Type::Float, 3);
	}
}
