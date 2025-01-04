#pragma once

#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"
#include "structure/graphics/spk_geometry_2D.hpp"
#include "structure/graphics/texture/spk_sprite_sheet.hpp"
#include "structure/graphics/opengl/spk_program.hpp"
#include "structure/graphics/opengl/spk_buffer_set.hpp"
#include "structure/graphics/opengl/spk_sampler_object.hpp"
#include "structure/graphics/texture/spk_image.hpp"
#include "structure/graphics/renderer/spk_texture_renderer.hpp"

#include <stdexcept>

namespace spk
{
    class NineSliceRenderer
    {
    public:
        static constexpr int GRID_SIZE = 3;

    private:
        TextureRenderer _textureRenderer;

    public:
        NineSliceRenderer() = default;

		void clear();

        void prepare(const Geometry2D& p_geometry, float p_layer, const Vector2Int& p_cornerSize);

        void setSpriteSheet(const SafePointer<SpriteSheet>& p_spriteSheet);

        void validate();

        void render();
    };
}
