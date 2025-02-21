#include "structure/graphics/renderer/spk_nine_slice_renderer.hpp"

#include "structure/graphics/spk_viewport.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
	void NineSliceRenderer::clear()
	{
		_textureRenderer.clear();
	}

	void NineSliceRenderer::prepare(const Geometry2D& p_geometry, float p_layer, const Vector2Int& p_cornerSize)
    {
		Vector2Int cornerSize = p_cornerSize;
        if (cornerSize.x * 2 > p_geometry.width || cornerSize.y * 2 > p_geometry.height)
        {
			cornerSize = { 0, 0 };
        }

        int xValues[GRID_SIZE + 1] = {
            0,
            cornerSize.x,
            static_cast<int>(p_geometry.width) - cornerSize.x,
            static_cast<int>(p_geometry.width)
        };

        int yValues[GRID_SIZE + 1] = {
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

                _textureRenderer.prepare(
                    subGeom,
                    section,
                    p_layer
                );
            }
        }
    }

	void NineSliceRenderer::setSpriteSheet(const SafePointer<SpriteSheet>& p_spriteSheet)
	{
		if (p_spriteSheet != nullptr && (p_spriteSheet->nbSprite().x != GRID_SIZE || p_spriteSheet->nbSprite().y != GRID_SIZE))
		{
			throw std::runtime_error("Invalid sprite sheet size. NineSlicedTextureRenderer expects a 3x3 sprite sheet.");
		}

		_textureRenderer.setTexture(safe_pointer_cast<OpenGL::TextureObject>(p_spriteSheet));
	}

	spk::SafePointer<spk::OpenGL::TextureObject>& NineSliceRenderer::spriteSheet()
	{
		return (_textureRenderer.texture());
	}
	
	const spk::SafePointer<spk::OpenGL::TextureObject>& NineSliceRenderer::spriteSheet() const
	{
		return (_textureRenderer.texture());
	}

	void NineSliceRenderer::validate()
	{
		_textureRenderer.validate();
	}

	void NineSliceRenderer::render()
	{
		_textureRenderer.render();
	}
}