#pragma once

#include "graphics/pipeline/spk_pipeline.hpp"
#include "graphics/spk_color.hpp"

namespace spk::widget::components
{
    class ColoredBox
    {
    private:
        static const std::string _renderingPipelineCode;
        static spk::Pipeline _renderingPipeline;

        struct ShaderInput
        {
            spk::Vector2Int position;

            ShaderInput(const spk::Vector2Int& p_position);
        };

        spk::Pipeline::Object _renderingObject;
        spk::Pipeline::Object::Attribute& _selfAttribute;
        spk::Pipeline::Object::Attribute::Element& _selfColorElement;
        spk::Pipeline::Object::Attribute::Element& _selfDepthElement;

        spk::Vector2 _anchor;
        spk::Vector2 _size;

        bool _needGPUInputUpdate;

        void _updateGPUData();

    public:
        ColoredBox();

        void render();

        const Vector2& anchor() const;
        const Vector2& size() const;

        void setGeometry(const Vector2 p_anchor, const Vector2 p_newSize);
        void setDepth(float p_newDepth);
        void setColor(const spk::Color& p_color);
    };
}