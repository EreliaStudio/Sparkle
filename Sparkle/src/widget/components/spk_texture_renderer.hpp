#pragma once

#include "graphics/pipeline/spk_pipeline.hpp"
#include "graphics/spk_color.hpp"

namespace spk::widget::components
{
    class TextureRenderer
    {
    private:
        static const std::string _renderingPipelineCode;
        static spk::Pipeline _renderingPipeline;

        struct ShaderInput
        {
            spk::Vector2Int position;
            spk::Vector2 uvs;

            ShaderInput(const spk::Vector2Int& p_position, const spk::Vector2& p_uvs);
        };

        spk::Pipeline::Object _renderingObject;
        spk::Pipeline::Object::Attribute& _selfAttribute;
        spk::Pipeline::Object::Attribute::Element& _selfLayerElement;
		spk::Pipeline::Texture& _renderingPipelineTexture;

        spk::Vector2 _anchor;
        spk::Vector2 _size;

		const spk::Texture* _texture;
		spk::Vector2 _uvAnchor;
		spk::Vector2 _uvSize;

        bool _needGPUInputUpdate;

        void _updateGPUData();

    public:
        TextureRenderer();

        void render();

		void setTexture(const spk::Texture* p_texture);
		void setTextureGeometry(const spk::Vector2& p_uvAnchor, const spk::Vector2& p_uvSize);

        const Vector2& anchor() const;
        const Vector2& size() const;

        const Vector2& uvAnchor() const;
        const Vector2& uvSize() const;

        void setGeometry(const Vector2& p_anchor, const Vector2& p_newSize);
        void setLayer(const float& p_layer);
    };
}