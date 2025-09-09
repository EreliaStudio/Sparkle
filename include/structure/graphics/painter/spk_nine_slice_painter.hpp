#pragma once

#include "structure/graphics/opengl/spk_buffer_set.hpp"
#include "structure/graphics/opengl/spk_program.hpp"
#include "structure/graphics/opengl/spk_sampler_object.hpp"
#include "structure/graphics/painter/spk_texture_painter.hpp"
#include "structure/graphics/spk_geometry_2d.hpp"
#include "structure/graphics/texture/spk_image.hpp"
#include "structure/graphics/texture/spk_sprite_sheet.hpp"
#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"

#include <stdexcept>

namespace spk
{
	class NineSlicePainter
	{
	public:
		static constexpr int GRID_SIZE = 3;

	private:
		TexturePainter _textureRenderer;

	public:
		NineSlicePainter() = default;

		void clear();

		void prepare(const Geometry2D &p_geometry, float p_layer, const Vector2Int &p_cornerSize);

		void setSpriteSheet(const SafePointer<const SpriteSheet> &p_spriteSheet);

		const spk::SafePointer<const SpriteSheet> &spriteSheet() const;

		void validate();

		void render();
	};
}
