#pragma once

#include "structure/engine/spk_color_vertex_2d.hpp"
#include "structure/engine/spk_mesh.hpp"

namespace spk
{
	class ColorMesh2D : public TMesh<ColorVertex2D>
	{
	public:
		ColorMesh2D() = default;
	};
}
