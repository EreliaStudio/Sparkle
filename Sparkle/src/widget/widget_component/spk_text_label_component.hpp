#pragma once

#include <string>
#include "math/spk_vector2.hpp"
#include "graphics/spk_color.hpp"
#include "graphics/pipeline/spk_pipeline.hpp"
#include "graphics/texture/font/spk_font.hpp"

namespace spk::WidgetComponent
{
    /**
     * @brief A component for rendering text within the UI, supporting various alignments, styles, and outlines.
     *
     * TextLabel is a versatile UI component designed for displaying text within the SPK engine's widget system.
     * It supports custom fonts, color, outlines, and alignment, making it suitable for a wide range of UI text rendering
     * tasks. The component utilizes the SPK engine's rendering pipeline to efficiently render text using font atlases,
     * ensuring high performance and quality.
     *
     * The class allows for detailed customization of text appearance, including font selection, size, color, outline style,
     * and color. It provides alignment options both vertically (Top, Centered, Bottom) and horizontally (Left, Centered,
     * Right), enabling precise control over text layout within its bounding box.
     *
     * Usage example:
     * @code
     * class MyCustomWidget : public spk::IWidget
     * {
     * private:
     *     spk::WidgetComponent::TextLabel _label;
     *
     *     void _onGeometryChange() 
     *     {
     *         _label.setAnchor(spk::Vector2Int(0, 0));
     *     }
     * 
     *     void _onRender() override
     *     {
     *         _label.render();
     *     }
     *
     *     void _onUpdate()
     *     {
     * 
     *     }
     *
     * public:
     *     MyCustomWidget(const std::string& name, spk::IWidget* parent = nullptr) :
     *         spk::IWidget(name, parent),
     *         _label()
     *     {
     *         _label.setFont(myFont); // Assuming myFont is a preloaded Font instance
     *         _label.setText("Hello, SPK!");
     *         _label.setTextSize(24);
     *         _label.setTextColor(spk::Color::White);
     *         _label.setAnchor({10, 10});
     *         _label.setVerticalAlignment(spk::VerticalAlignment::Centred);
     *         _label.setHorizontalAlignment(spk::HorizontalAlignment::Centred);
     *     }
     * };
     * @endcode
     *
     * @see Font, Pipeline
     */
    class TextLabel
    {
    public:
        enum class VerticalAlignment
        {   
            Top,
            Centred,
            Bottom
        };
        
        enum class HorizontalAlignment
        {
            Left,
            Centred,
            Right
        };

    private:
        struct RenderingPipelineVertex
        {
            spk::Vector2Int position;
            spk::Vector2 uvs;

            RenderingPipelineVertex();
            RenderingPipelineVertex(const spk::Vector2Int& p_position, const spk::Vector2& p_uvs);
        };

        struct RenderingData
        {
            std::vector<const spk::Font::Atlas::GlyphData*> glyphs;
            spk::Vector2Int size = spk::Vector2Int(0, 0);
        };

        static const std::string _renderingPipelineCode;
        static spk::Pipeline _renderingPipeline;

        spk::Pipeline::Object _renderingObject;
        spk::Pipeline::Object::Attribute& _renderingObjectWidgetAttribute;
        spk::Pipeline::Object::Attribute::Element& _depthAttributeElement;

        spk::Pipeline::Object::Attribute& _renderingObjectTextRendererAttribute;
        spk::Pipeline::Object::Attribute::Element& _textColorAttributeElement;
        spk::Pipeline::Object::Attribute::Element& _outlineColorAttributeElement;
		
		spk::Pipeline::Texture& _renderingPipelineTexture;

        bool _needGPUBufferUpdate = true;

        spk::Font* _font = nullptr;
        spk::Font::Atlas* _fontAtlas = nullptr;

        std::string _text = "";

        size_t _textSize = 1;
        spk::Color _textColor = spk::Color(0, 0, 0);

        spk::Font::OutlineStyle _outlineStyle = spk::Font::OutlineStyle::Standard;
        size_t _outlineSize = 0;
        spk::Color _outlineColor = spk::Color(255, 255, 255);

        spk::Vector2Int _anchor;

        VerticalAlignment _verticalAlignment = VerticalAlignment::Centred;
        HorizontalAlignment _horizontalAlignment = HorizontalAlignment::Centred;

        TextLabel::RenderingData _computeRenderingData();
        spk::Vector2Int _computeBaseAnchor(const TextLabel::RenderingData& p_renderingData);

        void _updateGPUBuffer();
        
    public:
        TextLabel();

        void render();

        void setFont(spk::Font* p_font);
        void setAnchor(const spk::Vector2Int& p_anchor);
        void setDepth(const float& p_depth);
        void setText(const std::string& p_text);
        void setTextSize(const size_t& p_textSize);
        void setTextColor(const spk::Color& p_textColor);
        void setOutlineStyle(const spk::Font::OutlineStyle& p_outlineStyle);
        void setOutlineSize(const size_t& p_outlineSize);
        void setOutlineColor(const spk::Color& p_outlineColor);
        void setVerticalAlignment(const VerticalAlignment& p_verticalAlignment);
        void setHorizontalAlignment(const HorizontalAlignment& p_horizontalAlignment);
    };
}

namespace spk
{
    using VerticalAlignment = WidgetComponent::TextLabel::VerticalAlignment;
    using HorizontalAlignment = WidgetComponent::TextLabel::HorizontalAlignment;
}