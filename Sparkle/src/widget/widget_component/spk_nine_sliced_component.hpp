#pragma once

#include <string>
#include "math/spk_vector2.hpp"
#include "graphics/pipeline/spk_pipeline.hpp"
#include "graphics/texture/spk_sprite_sheet.hpp"

namespace spk::WidgetComponent
{
    class NineSlicedBox
    {
    private:
        static std::string _renderingPipelineCode;

        struct RenderingPipelineVertex
        {
            spk::Vector2Int position;
            spk::Vector2 uvs;

            RenderingPipelineVertex();
            RenderingPipelineVertex(const spk::Vector2Int& p_position, const spk::Vector2& p_uvs);
        };

        static spk::Pipeline _renderingPipeline;
        spk::Pipeline::Object _renderingObject;
        spk::Pipeline::Object::Attribute& _renderingObjectAttribute;
        spk::Pipeline::Object::Attribute::Element& _depthAttributeElement;
		
		spk::Pipeline::Texture& _renderingPipelineTexture;

        const spk::SpriteSheet* _spriteSheet;

        spk::Vector2Int _anchor;
        spk::Vector2UInt _size;
        spk::Vector2Int _cornerSize;

        bool _needGPUUpdate;

        void _updateVertices();

    public:
        NineSlicedBox();

        void setSpriteSheet(const spk::SpriteSheet* p_spriteSheet);
        void setGeometry(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size);
        void setDepth(float p_depth);
        void setCornerSize(const spk::Vector2Int& p_cornerSize);

        void render();
    };
}