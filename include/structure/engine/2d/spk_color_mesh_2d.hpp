#pragma once

#include "structure/engine/2d/spk_color_vertex_2d.hpp"
#include "structure/engine/spk_generic_mesh.hpp"

namespace spk
{
	class ColorMesh2D : public GenericMesh<ColorVertex2D>
	{
	public:
		ColorMesh2D() = default;
	};
}