#include "widget/component/spk_nine_slice_renderer.hpp"

namespace spk::WidgetComponent
{
    void NineSliceRenderer::_updateGPUData()
    {
        struct Vertex
        {
            spk::Vector2Int position;
            spk::Vector2 uvs;
        };

        int pointsX[4] = {
            static_cast<int>(_geometry.anchor.x),
            static_cast<int>(_geometry.anchor.x + _cornerSize.x),
            static_cast<int>(_geometry.anchor.x + _geometry.size.x - _cornerSize.x),
            static_cast<int>(_geometry.anchor.x + _geometry.size.x)
        };

        int pointsY[4] = {
            static_cast<int>(_geometry.anchor.y),
            static_cast<int>(_geometry.anchor.y + _cornerSize.y),
            static_cast<int>(_geometry.anchor.y + _geometry.size.y - _cornerSize.y),
            static_cast<int>(_geometry.anchor.y + _geometry.size.y)
        };

        spk::Vector2 uvs[4] = {
            spk::Vector2(0.0f, 0.0f),
            spk::Vector2(0.0f, 1.0f),
            spk::Vector2(1.0f, 0.0f),
            spk::Vector2(1.0f, 1.0f)
        };

        unsigned int quadIndices[6] = { 0, 1, 2, 2, 1, 3 };

        // _object.layout().clear();
        // _object.indexes().clear();

        for (size_t x = 0; x < 3; x++)
        {
            for (size_t y = 0; y < 3; y++)
            {
                // unsigned int baseVertexIndex = static_cast<unsigned int>(_object.layout().size() / sizeof(Vertex));

                for (size_t i = 0; i < 2; i++)
                {
                    for (size_t j = 0; j < 2; j++)
                    {
                        Vertex newVertex;
                        newVertex.position = spk::Vector2Int(pointsX[x + i], pointsY[y + j]);

                        spk::SpriteSheet::Sprite sprite = _texture->sprite(spk::Vector2UInt(x, y));
                        size_t uvIndex = j + i * 2;

                        newVertex.uvs = sprite.anchor + sprite.size * uvs[uvIndex];

                        // _object.layout() << (newVertex);
                    }
                }

                for (size_t i = 0; i < 6; i++)
                {
                    // _object.indexes() << (baseVertexIndex + quadIndices[i]);
                }
            }
        }

        // _object.layout().validate();
        // _object.indexes().validate();
    }

    NineSliceRenderer::NineSliceRenderer()
    {
        // _layerElement = _layer;
        // _alphaElement = _alpha;

        // _textureInformations.validate();
    }

    const float& NineSliceRenderer::layer() const
    {
        return _layer;
    }

    const float& NineSliceRenderer::alpha() const
    {
        return _alpha;
    }

    const spk::Geometry2D& NineSliceRenderer::geometry() const
    {
        return _geometry;
    }

    const spk::SafePointer<spk::SpriteSheet>& NineSliceRenderer::texture() const
    {
        return _texture;
    }

    const spk::Vector2UInt& NineSliceRenderer::cornerSize() const
    {
        return _cornerSize;
    }

    void NineSliceRenderer::setLayer(const float& p_layer)
    {
        _layer = p_layer;
        // _layerElement = _layer;
        // _textureInformations.validate();
    }

    void NineSliceRenderer::setAlpha(const float& p_alpha)
    {
        _alpha = p_alpha;
        // _alphaElement = _alpha;
        // _textureInformations.validate();
    }

    void NineSliceRenderer::setGeometry(const spk::Geometry2D& p_geometry)
    {
        _geometry = p_geometry;
        _needUpdateGPU = true;
    }

    void NineSliceRenderer::setTexture(const spk::SafePointer<spk::SpriteSheet>& p_texture)
    {
        _texture = p_texture;
        // _textureSampler2D.bind(_texture);
    }

    void NineSliceRenderer::setCornerSize(const spk::Vector2UInt& p_cornerSize)
    {
        _cornerSize = p_cornerSize;
        _needUpdateGPU = true;
    }

    void NineSliceRenderer::render()
    {
        if (_needUpdateGPU)
        {
            _updateGPUData();
            _needUpdateGPU = false;
        }

        // _object.render();
    }
}
