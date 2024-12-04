#pragma once

#include "structure/engine/spk_generic_mesh.hpp"
#include "structure/engine/3d/spk_color_vertex.hpp"

namespace spk
{
	class ColorMesh : public GenericMesh<ColorVertex>
	{
	public:
		ColorMesh() = default;
	};
}
