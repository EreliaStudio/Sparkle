#include "structure/graphics/renderer/spk_nine_slice_renderer.hpp"

#include "structure/graphics/spk_viewport.hpp"

namespace spk
{
	void NineSliceRenderer::clear()
	{
		_textureRenderer.clear();
	}

	void NineSliceRenderer::prepare(const Geometry2D& p_geometry, float p_layer, const Vector2Int& p_cornerSize)
    {
        if (p_cornerSize.x * 2 > p_geometry.width || p_cornerSize.y * 2 > p_geometry.height)
        {
            throw std::invalid_argument("Corner size exceeds half of the geometry dimensions.");
        }

        int xValues[GRID_SIZE + 1] = {
            0,
            p_cornerSize.x,
            static_cast<int>(p_geometry.width) - p_cornerSize.x,
            static_cast<int>(p_geometry.width)
        };

        int yValues[GRID_SIZE + 1] = {
            0,
            p_cornerSize.y,
            static_cast<int>(p_geometry.height) - p_cornerSize.y,
            static_cast<int>(p_geometry.height)
        };

		float uStep = 1.0f / static_cast<float>(GRID_SIZE);
		float vStep = 1.0f / static_cast<float>(GRID_SIZE);

		float uValues[GRID_SIZE + 1] = {
			0.0f,
			uStep,
			2.0f * uStep,
			1.0f
		};

		float vValues[GRID_SIZE + 1] = {
			0.0f,
			vStep,
			2.0f * vStep,
			1.0f
		};

        for (int row = 0; row < GRID_SIZE; ++row)
        {
            for (int col = 0; col < GRID_SIZE; ++col)
            {
                Geometry2D subGeom;
                subGeom.x = xValues[col];
                subGeom.y = yValues[row];
                subGeom.width = xValues[col + 1] - xValues[col];
                subGeom.height = yValues[row + 1] - yValues[row];

                Image::Section section;
                section.x = xValues[col];
                section.y = yValues[row];
                section.width = xValues[col + 1] - xValues[col];
                section.height = yValues[row + 1] - yValues[row];

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
		if (p_spriteSheet->size().x != GRID_SIZE || p_spriteSheet->size().y != GRID_SIZE)
		{
			throw std::runtime_error("Invalid sprite sheet size. NineSlicedTextureRenderer expects a 3x3 sprite sheet.");
		}

		_textureRenderer.setTexture(p_spriteSheet);
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