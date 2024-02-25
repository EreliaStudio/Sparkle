#pragma once

#include "graphics/pipeline/spk_pipeline.hpp"
#include "graphics/spk_color.hpp"

namespace spk::widget::components
{
    /**
     * @brief Component to draw a color on a rectangle box.
     */
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
        /**
         * @brief Constructor.
         */
        ColoredBox();

        /**
         * @brief Render the box.
         */
        void render();

        /**
         * @brief Getter for anchor.
         */
        const Vector2& anchor() const;

        /**
         * @brief Getter for size.
         */
        const Vector2& size() const;

        /**
         * @brief Set the geometry of this component.
         * @param p_anchor The new anchor.
         * @param p_newSize The new size.
         */
        void setGeometry(const Vector2 p_anchor, const Vector2 p_newSize);

        /**
         * @brief Set the depth of this component.
         * @param p_newDepth The new depth.
         */
        void setDepth(float p_newDepth);

        /**
         * @brief Set the color of the box.
         * @param p_color The new color.
         */
        void setColor(const spk::Color& p_color);
    };
}