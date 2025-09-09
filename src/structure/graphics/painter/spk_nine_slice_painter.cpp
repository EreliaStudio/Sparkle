#include "structure/graphics/painter/spk_nine_slice_painter.hpp"

#include "structure/graphics/spk_viewport.hpp"

#include "spk_debug_macro.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	void NineSlicePainter::clear()
	{
		_textureRenderer.clear();
	}

	void NineSlicePainter::prepare(const Geometry2D &p_geometry, float p_layer, const Vector2Int &p_cornerSize)
	{
		Vector2Int cornerSize = p_cornerSize;
		if (cornerSize.x * 2 > p_geometry.width || cornerSize.y * 2 > p_geometry.height)
		{
			cornerSize = {0, 0};
		}

		std::array<int, GRID_SIZE + 1> xValues = {
			0, cornerSize.x, static_cast<int>(p_geometry.width) - cornerSize.x, static_cast<int>(p_geometry.width)};

		std::array<int, GRID_SIZE + 1> yValues = {
			0,
			cornerSize.y,
			static_cast<int>(p_geometry.height) - cornerSize.y,
			static_cast<int>(p_geometry.height),
		};

		float uStep = 1.0f / static_cast<float>(GRID_SIZE);
		float vStep = 1.0f / static_cast<float>(GRID_SIZE);

		for (int row = 0; row < GRID_SIZE; ++row)
		{
			for (int col = 0; col < GRID_SIZE; ++col)
			{
				Geometry2D subGeom;
				subGeom.x = p_geometry.x + xValues[col];
				subGeom.y = p_geometry.y + yValues[row];
				subGeom.width = xValues[col + 1] - xValues[col];
				subGeom.height = yValues[row + 1] - yValues[row];

				Image::Section section;
				section.x = uStep * col;
				section.y = vStep * (row);
				section.width = uStep;
				section.height = vStep;

				_textureRenderer.prepare(subGeom, section, p_layer);
			}
		}
	}

	void NineSlicePainter::setSpriteSheet(const SafePointer<const SpriteSheet> &p_spriteSheet)
	{
		if (p_spriteSheet != nullptr && (p_spriteSheet->nbSprite().x != GRID_SIZE || p_spriteSheet->nbSprite().y != GRID_SIZE))
		{
			GENERATE_ERROR("Invalid sprite sheet size. NineSlicedTextureRenderer expects a 3x3 sprite sheet.");
		}

		_textureRenderer.setTexture(p_spriteSheet.downCast<spk::Texture>());
	}

	const spk::SafePointer<const SpriteSheet> &NineSlicePainter::spriteSheet() const
	{
		return (_textureRenderer.texture().upCast<const SpriteSheet>());
	}

	void NineSlicePainter::validate()
	{
		_textureRenderer.validate();
	}

	void NineSlicePainter::render()
	{
		_textureRenderer.render();
	}
}