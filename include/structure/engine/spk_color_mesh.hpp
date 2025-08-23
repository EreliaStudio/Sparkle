#pragma once

#include "structure/engine/spk_color_vertex.hpp"
#include "structure/engine/spk_mesh.hpp"

namespace spk
{
	class ColorMesh : public TMesh<ColorVertex>
	{
	public:
		ColorMesh() = default;
	};
}
