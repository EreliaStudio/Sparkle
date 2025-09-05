#pragma once

#include "structure/engine/spk_mesh_2d.hpp"
#include "structure/graphics/texture/spk_sprite_sheet.hpp"
#include "structure/math/spk_vector2.hpp"

namespace spk
{
	namespace Primitive2D
	{
		spk::Mesh2D makeSquare(const spk::Vector2 &p_size, const spk::SpriteSheet::Section &p_uv);
		spk::Mesh2D makeCircle(float p_radius, size_t p_pointCount, const spk::SpriteSheet::Section &p_uv);
	}
}
