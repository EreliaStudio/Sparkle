#include "widget/components/spk_text_label.hpp"

namespace spk::widget::components
{
    const std::string TextLabel::_renderingPipelineCode = R"(#version 450
    
	#include <widgetAttribute>
	#include <screenConstants>

    AttributeBlock textRendererAttribute
    {
        vec4 textColor;
        vec4 outlineColor;
    };
    
	Input -> Geometry : ivec2 screenPosition;
	Input -> Geometry : vec2 UVs;
	Geometry -> Render : vec2 fragmentUVs;

	Texture fontTexture;

    void geometryPass()
    {
		pixelPosition = screenConstants.canvasMVP * vec4(screenPosition, widgetAttribute.depth, 1.0);

		fragmentUVs = UVs;
    }
    
    void renderPass()
    {
        float r = texture(fontTexture, fragmentUVs).r;

        pixelColor = vec4(r, r, r, r);
        // if (r == 0.0)
        // {
        //     discard;
        // }
        // else if (r == 1.0)
        // {
        //     pixelColor = textRendererAttribute.textColor;
        // }
        // else
        // {
        //     pixelColor = textRendererAttribute.outlineColor;
        // }
    })";
    spk::Pipeline TextLabel::_renderingPipeline = spk::Pipeline(TextLabel::_renderingPipelineCode);

    TextLabel::TextLabel() :
        _renderingObject(_renderingPipeline.createObject()),
        _renderingObjectWidgetAttribute(_renderingObject.attribute("widgetAttribute")),
        _depthAttributeElement(_renderingObjectWidgetAttribute["depth"]),
        _renderingObjectTextRendererAttribute(_renderingObject.attribute("textRendererAttribute")),
        _textColorAttributeElement(_renderingObjectTextRendererAttribute["textColor"]),
        _outlineColorAttributeElement(_renderingObjectTextRendererAttribute["outlineColor"]),
        _renderingPipelineTexture(_renderingPipeline.texture("fontTexture"))
    {
    }

    TextLabel::ShaderInput::ShaderInput() :
        position(0),
        uvs(0)
    {
    }

    TextLabel::ShaderInput::ShaderInput(const spk::Vector2Int& p_position, const spk::Vector2& p_uvs) :
        position(p_position),
        uvs(p_uvs)
    {
    }

    TextLabel::RenderingData TextLabel::_computeRenderingData()
    {
        RenderingData result;

        spk::Vector2Int topLeftCorner = 0;
        spk::Vector2Int downRightCorner = 0;

        spk::Vector2Int stringOffset = 0;
        for (size_t i = 0; i < _text.size(); i++)
        {
            const Font::Atlas::GlyphData& glyphData = (*_fontAtlas)[_text[i]];

            if (topLeftCorner.y > glyphData.position[0].y)
                topLeftCorner.y = glyphData.position[0].y;
            stringOffset += glyphData.step;

            result.glyphs.push_back(&glyphData);
        }

        downRightCorner = stringOffset;

        result.size = downRightCorner - topLeftCorner;

        return (result);
    }

    spk::Vector2Int TextLabel::_computeBaseAnchor(const TextLabel::RenderingData& p_renderingData)
    {
        spk::Vector2Int result = 0;

        switch (_horizontalAlignment)
        {
        case HorizontalAlignment::Left:
        {
            result.x = 0;
            break;
        }

        case HorizontalAlignment::Centered:
        {
            result.x = -(p_renderingData.size.x / 2);
            break;
        }

        case HorizontalAlignment::Right:
        default:
        {
            result.x = -p_renderingData.size.x;
            break;
        }
        }

        switch (_verticalAlignment)
        {
        case VerticalAlignment::Top:
        {
            result.y = p_renderingData.size.y;
            break;
        }

        case VerticalAlignment::Centered:
        {
            result.y = p_renderingData.size.y / 2;
            break;
        }

        case VerticalAlignment::Down:
        default:
        {
            result.y = 0;
            break;
        }
        }

        return (result);
    }

    void TextLabel::_updateGPUBuffer()
    {
        _fontAtlas = &(_font->atlas(_textSize, _outlineSize, _outlineStyle));

        _renderingPipelineTexture.attach(&(_fontAtlas->texture()));

        static std::vector<ShaderInput> _bufferShaderInput;
        static std::vector<unsigned int> _bufferRenderingPipelineIndexes;

        _bufferShaderInput.clear();
        _bufferRenderingPipelineIndexes.clear();

        RenderingData renderingData = _computeRenderingData();

        spk::Vector2Int glyphAnchor = _anchor + _computeBaseAnchor(renderingData);

        for (const spk::Font::Atlas::GlyphData* glyphData : renderingData.glyphs)
        {
            unsigned int baseIndexes = _bufferShaderInput.size();

            for (size_t i = 0; i < 4; i++)
            {
                ShaderInput newVertex;

                newVertex.position = glyphAnchor + glyphData->position[i];
                newVertex.uvs = glyphData->uvs[i];

                _bufferShaderInput.push_back(newVertex);
            }
            glyphAnchor += glyphData->step;

            for (size_t i = 0; i < 6; i++)
            {
                _bufferRenderingPipelineIndexes.push_back(baseIndexes + spk::Font::Atlas::GlyphData::indexesOrder[i]);
            }
        }

        _renderingObject.setVertices(_bufferShaderInput);
        _renderingObject.setIndexes(_bufferRenderingPipelineIndexes);
    }

    void TextLabel::render()
    {
        if (_font == nullptr)
            return;

        if (_needGPUBufferUpdate == true)
        {
            _updateGPUBuffer();
            _needGPUBufferUpdate = false;
        }

        if (_fontAtlas == nullptr)
            return;

        _renderingPipelineTexture.attach(&(_fontAtlas->texture()));
        _renderingObject.render();
    }

    void TextLabel::setFont(spk::Font* p_font)
    {
        _font = p_font;
        _needGPUBufferUpdate = true;
    }

    void TextLabel::setAnchor(const spk::Vector2Int& p_anchor)
    {
        _anchor = p_anchor;
        _needGPUBufferUpdate = true;
    }

    void TextLabel::setDepth(const float& p_depth)
    {
        _depthAttributeElement = p_depth;
        _renderingObjectWidgetAttribute.update();
    }

    void TextLabel::setText(const std::string& p_text)
    {
        _text = p_text;
        _needGPUBufferUpdate = true;
    }

    void TextLabel::setTextSize(const size_t& p_textSize)
    {
        _textSize = p_textSize;
        _needGPUBufferUpdate = true;
    }

    void TextLabel::setTextColor(const spk::Color& p_textColor)
    {
        _textColorAttributeElement = p_textColor;
        _renderingObjectTextRendererAttribute.update();
    }

    void TextLabel::setOutlineStyle(const spk::Font::OutlineStyle& p_outlineStyle)
    {
        _outlineStyle = p_outlineStyle;
        _needGPUBufferUpdate = true;
    }

    void TextLabel::setOutlineSize(const size_t& p_outlineSize)
    {
        _outlineSize = p_outlineSize;
        _needGPUBufferUpdate = true;
    }

    void TextLabel::setOutlineColor(const spk::Color& p_outlineColor)
    {
        _outlineColorAttributeElement = p_outlineColor;
        _renderingObjectTextRendererAttribute.update();
    }

    void TextLabel::setVerticalAlignment(const VerticalAlignment& p_verticalAlignment)
    {
        _verticalAlignment = p_verticalAlignment;
        _needGPUBufferUpdate = true;
    }

    void TextLabel::setHorizontalAlignment(const HorizontalAlignment& p_horizontalAlignment)
    {
        _horizontalAlignment = p_horizontalAlignment;
        _needGPUBufferUpdate = true;
    }
}
