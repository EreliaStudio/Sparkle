#pragma once

#include "structure/engine/spk_mesh.hpp"
#include "structure/engine/spk_vertex_2D.hpp"

namespace spk
{
	class Mesh2D : public spk::TMesh<spk::Vertex2D>
	{
	public:
		Mesh2D() = default;
	};
}
