#pragma once

#include "structure/engine/spk_generic_mesh.hpp"
#include "structure/engine/2d/spk_vertex_2d.hpp"

namespace spk
{
	class Mesh2D : public spk::GenericMesh<spk::Vertex2D>
	{
	public:
		Mesh2D() = default;
	};
}
