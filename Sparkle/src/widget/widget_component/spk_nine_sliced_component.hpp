#pragma once

#include <string>
#include "math/spk_vector2.hpp"
#include "graphics/pipeline/spk_pipeline.hpp"
#include "graphics/texture/spk_sprite_sheet.hpp"

namespace spk::WidgetComponent
{
    /**
     * @brief Implements a nine-sliced box component for UI rendering, supporting scalable and adaptable layouts.
     *
     * NineSlicedBox utilizes a nine-slice technique to render scalable UI elements where the corners remain
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
            RenderingPipelineVertex(const spk::Vector2Int &p_position, const spk::Vector2 &p_uvs);
        };

        static spk::Pipeline _renderingPipeline;
        spk::Pipeline::Object _renderingObject;
        spk::Pipeline::Object::Attribute &_renderingObjectAttribute;
        spk::Pipeline::Object::Attribute::Element &_depthAttributeElement;

        spk::Pipeline::Texture &_renderingPipelineTexture;

        const spk::SpriteSheet *_spriteSheet;

        spk::Vector2Int _anchor;
        spk::Vector2UInt _size;
        spk::Vector2Int _cornerSize;

        bool _needGPUUpdate;

        void _updateVertices();

    public:
        NineSlicedBox();

        void setSpriteSheet(const spk::SpriteSheet *p_spriteSheet);
        void setGeometry(const spk::Vector2Int &p_anchor, const spk::Vector2UInt &p_size);
        void setDepth(float p_depth);
        void setCornerSize(const spk::Vector2Int &p_cornerSize);

        void render();
    };
}