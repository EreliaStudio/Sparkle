#pragma once

#include "graphics/pipeline/spk_pipeline.hpp"
#include "graphics/texture/spk_sprite_sheet.hpp"
#include "math/spk_vector2.hpp"
#include <string>

namespace spk::widget::components
{
    /**
     * @class NineSlicedBox
     * @brief Implements a nine-sliced box component for UI rendering, supporting scalable and adaptable layouts.
     *
     * This class utilizes a nine-slice technique to render scalable UI elements where the corners remain
     * unscaled, preserving the element's visual integrity at various sizes. This approach is particularly
     * useful for UI backgrounds, buttons, and other interface elements that need to dynamically resize without
     * distorting their graphical content.
     *
     * The class manages its own rendering pipeline and texture, leveraging the spk::Pipeline and spk::SpriteSheet
     * classes for efficient GPU rendering. It is designed to work within the SPK engine's widget component system,
     * providing a high level of integration with other UI elements and the overall rendering process.
     *
     * Key features include:
     * - Customizable geometry and size, allowing for flexible integration into various UI layouts.
     * - Support for sprite sheets, enabling the use of texture atlases for efficient rendering.
     * - Dynamic GPU resource management, ensuring that vertex and texture data are updated as needed.
     *
     * Usage example:
     * @code
     * class MyCustomWidget : public spk::IWidget
     * {
     * private:
     *     spk::WidgetComponent::NineSlicedBox _background;
     *
     *     void _onGeometryChange()
     *     {
     *         _background.setGeometry(spk::Vector2Int(0, 0), size());
     *     }
     *
     *     void _onRender()
     *     {
     *         _background.render();
     *     }
     *
     *     void _onUpdate()
     *     {
     *
     *     }
     * public:
     *     MyCustomWidget(const std::string& name, spk::IWidget* parent = nullptr) :
     *          spk::IWidget(name, parent),
     *          _background()
     *     {
     *         _background.setSpriteSheet(&spriteSheet);
     *         _background.setCornerSize(spk::Vector2Int(32, 32)); // Set the size of the corners
     *     }
     * };
     * @endcode
     *
     * @note This component requires initialization and setup of a rendering pipeline and sprite sheet before use.
     * It is designed to seamlessly integrate with the SPK engine's rendering system, providing efficient and
     * flexible UI rendering capabilities.
     *
     * @see IVector2, SpriteSheet
     */
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
        /**
         * @brief Constructs a NineSlicedBox component with default settings.
         *
         * Initializes the nine-sliced box with default values, preparing it for configuration and rendering. This
         * includes setting up initial geometry, size, and corner size. The actual rendering pipeline and sprite sheet
         * must be set separately using the provided methods.
         */
        NineSlicedBox();

        /**
         * @brief Sets the sprite sheet used for rendering the nine-sliced box.
         *
         * @param p_spriteSheet Pointer to the SpriteSheet object that contains the textures for the nine-sliced box.
         */
        void setSpriteSheet(const spk::SpriteSheet* p_spriteSheet);

        /**
         * @brief Sets the geometry of the nine-sliced box.
         *
         * Defines the anchor point and size of the nine-sliced box. The anchor point determines the box's position,
         * while the size specifies its dimensions.
         *
         * @param p_anchor The position of the nine-sliced box within its parent container.
         * @param p_size The size of the nine-sliced box.
         */
        void setGeometry(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size);

        /**
         * @brief Sets the rendering depth of the nine-sliced box in the pipeline.
         *
         * @param p_depth A float value representing the depth at which the box should be rendered.
         */
        void setDepth(float p_depth);

        /**
         * @brief Sets the size of the corners of the nine-sliced box.
         *
         * This size affects how the corners of the box are rendered, ensuring they remain unscaled when the box is resized.
         *
         * @param p_cornerSize The size of the box's corners.
         */
        void setCornerSize(const spk::Vector2Int& p_cornerSize);

        /**
         * @brief Renders the nine-sliced box.
         *
         * Executes the rendering process for the nine-sliced box, updating its geometry and texture coordinates as necessary
         * before drawing it to the screen. This method should be called within the parent widget's rendering cycle.
         */
        void render();
    };
}