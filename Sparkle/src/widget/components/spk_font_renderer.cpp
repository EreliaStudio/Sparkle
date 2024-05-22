#include "widget/components/spk_font_renderer.hpp"

#include <iostream>

namespace spk::WidgetComponent
{
	std::string FontRenderer::_renderingPipelineCode = R"(#version 450
		#include <widgetAttribute>
		#include <screenConstants>

		Input -> Geometry : ivec2 modelPosition;
		Input -> Geometry : vec2 modelUVs;

		Geometry -> Render : vec2 fragmentUVs;

		AttributeBlock textRendererAttribute
		{
			vec4 textColor;
			float textEdgePower;
			vec4 outlineColor;
			float outlineEdgePower;
		};

		Texture fontTexture;

		void geometryPass()
		{
			pixelPosition = screenConstants.canvasMVP * vec4(modelPosition, widgetAttribute.layer, 1.0f);

			fragmentUVs = modelUVs;
		}

		float strongInterpolation(float x, float minRange, float maxRange, float exponent)
		{
			float adjustedX = (x - minRange) / (maxRange - minRange);
			float strongValue = pow(adjustedX, exponent);
			return strongValue;
		}

		void renderPass()
		{
			float distance = texture(fontTexture, fragmentUVs).r;

			if (distance <= 0.00390f)
			{
				pixelColor = vec4(1, 0, 1, 1);
				// discard;
			}
			else
			{
				vec4 resultColor;
				if (distance >= 0.5)
				{
					float t = strongInterpolation(distance, 0.5, 1.0, textRendererAttribute.textEdgePower);
					resultColor = mix(textRendererAttribute.outlineColor, textRendererAttribute.textColor, t);
				}
				else
				{
					float alpha = strongInterpolation(distance, 0.0, 0.5, textRendererAttribute.outlineEdgePower);
					resultColor = vec4(textRendererAttribute.outlineColor.rgb, alpha);
				}

				pixelColor = resultColor;
				//pixelColor = vec4(distance, distance, distance, 1.0f);
			}
		})";

	spk::Pipeline FontRenderer::_renderingPipeline = spk::Pipeline(_renderingPipelineCode);

	FontRenderer::ShaderInput::ShaderInput() :
		ShaderInput(0, 0)
	{

	}

	FontRenderer::ShaderInput::ShaderInput(const spk::Vector2Int& p_position, const spk::Vector2& p_uvs) :
		position(p_position),
		uvs(p_uvs)
	{

	}

	FontRenderer::FontRenderer() :
		_renderingObject(_renderingPipeline.createObject()),
		_renderingObjectWidgetAttribute(_renderingObject.attribute("widgetAttribute")),
		_renderingObjectWidgetAttributeLayerElement(_renderingObjectWidgetAttribute["layer"]),
		_renderingObjectFontRendererAttribute(_renderingObject.attribute("textRendererAttribute")),
		_renderingObjectFontRendererAttributeTextColorElement(_renderingObjectFontRendererAttribute["textColor"]),
		_renderingObjectFontRendererAttributeTextEdgePowerElement(_renderingObjectFontRendererAttribute["textEdgePower"]),
		_renderingObjectFontRendererAttributeOutlineColorElement(_renderingObjectFontRendererAttribute["outlineColor"]),
		_renderingObjectFontRendererAttributeOutlineEdgePowerElement(_renderingObjectFontRendererAttribute["outlineEdgePower"]),
		_renderingPipelineTexture(_renderingPipeline.texture("fontTexture")),
		_font(nullptr),
		_needGPUBufferUpdate(false),
		_anchor(0, 0),
		_size(0, 0),
		_verticalAlignment(spk::VerticalAlignment::Centered),
		_horizontalAlignment(spk::HorizontalAlignment::Centered),
		_textSize(0),
		_textEdgeStrenght(20.0f),
		_outlineSize(0),
		_outlineEdgeStrenght(20.0f)
	{
		setTextEdgeStrenght(20.0f);
		setOutlineEdgeStrenght(20.0f);
	}

	void FontRenderer::render()
	{
		if (_font == nullptr)
			return ;

		if (_needGPUBufferUpdate == true)
		{
			_updateGPUData();
			_needGPUBufferUpdate = false;
		}

		_renderingPipelineTexture.attach(_fontAtlas);
		_renderingObject.render();
	}

	void FontRenderer::setFont(spk::Font* p_font)
	{
		_font = p_font;
		_needGPUBufferUpdate = true;
	}

	void FontRenderer::setText(const std::string& p_text)
	{
		_text = p_text;
		_needGPUBufferUpdate = true;
	}

	void FontRenderer::setTextColor(const spk::Color& p_textColor)
	{
		_textColor = p_textColor;
		_renderingObjectFontRendererAttributeTextColorElement = p_textColor;
		_renderingObjectFontRendererAttribute.update();
	}

	void FontRenderer::setTextSize(const size_t& p_textSize)
	{
		_textSize = p_textSize;
		_needGPUBufferUpdate = true;
	}

	void FontRenderer::setTextEdgeStrenght(const float& p_textEdgeStrenght)
	{
		_textEdgeStrenght = p_textEdgeStrenght;
		_renderingObjectFontRendererAttributeTextEdgePowerElement = p_textEdgeStrenght;
		_renderingObjectFontRendererAttribute.update();
	}

	void FontRenderer::setOutlineColor(const spk::Color& p_outlineColor)
	{
		_outlineColor = p_outlineColor;
		_renderingObjectFontRendererAttributeOutlineColorElement = p_outlineColor;
		_renderingObjectFontRendererAttribute.update();
	}

	void FontRenderer::setOutlineSize(const size_t& p_outlineSize)
	{
		_outlineSize = p_outlineSize;
		_needGPUBufferUpdate = true;
	}

	void FontRenderer::setOutlineEdgeStrenght(const float& p_outlineEdgeStrenght)
	{
		_outlineEdgeStrenght = p_outlineEdgeStrenght;
		_renderingObjectFontRendererAttributeOutlineEdgePowerElement = p_outlineEdgeStrenght;
		_renderingObjectFontRendererAttribute.update();
	}

	void FontRenderer::setVerticalAlignment(const spk::VerticalAlignment& p_verticalAlignment)
	{
		_verticalAlignment = p_verticalAlignment;
		_needGPUBufferUpdate = true;
	}

	void FontRenderer::setHorizontalAlignment(const spk::HorizontalAlignment& p_horizontalAlignment)
	{
		_horizontalAlignment = p_horizontalAlignment;
		_needGPUBufferUpdate = true;
	}

	spk::Font* FontRenderer::font() const
	{
		return (_font);
	}

	const std::string& FontRenderer::text() const
	{
		return (_text);
	}

	const spk::Vector2Int& FontRenderer::anchor() const
	{
		return _anchor;
	}

	const spk::Vector2Int& FontRenderer::size() const
	{
		return _size;
	}

	size_t FontRenderer::textSize() const
	{
		return (_textSize);
	}

	const spk::Color& FontRenderer::textColor() const
	{
		return (_textColor);
	}

	const float& FontRenderer::textEdgeStrenght() const
	{
		return (_textEdgeStrenght);
	}

	size_t FontRenderer::outlineSize() const
	{
		return (_outlineSize);
	}

	const spk::Color& FontRenderer::outlineColor() const
	{
		return (_outlineColor);
	}

	const float& FontRenderer::outlineEdgeStrenght() const
	{
		return (_outlineEdgeStrenght);
	}

	const spk::VerticalAlignment& FontRenderer::verticalAlignment() const
	{
		return (_verticalAlignment);
	}

	const spk::HorizontalAlignment& FontRenderer::horizontalAlignment() const
	{
		return (_horizontalAlignment);
	}

	void FontRenderer::setLayer(const float& p_layer)
	{
		_renderingObjectWidgetAttributeLayerElement = p_layer;
		_renderingObjectWidgetAttribute.update();
	}

	void FontRenderer::setGeometry(const spk::Vector2Int& p_anchor, const spk::Vector2Int& p_newSize)
	{
		_anchor = p_anchor;
		_size = p_newSize;
		_needGPUBufferUpdate = true;
	}

	FontRenderer::RenderingData FontRenderer::_computeRenderingData(spk::Font::Atlas* p_fontAtlas, const std::string& p_text) const
	{
		RenderingData result;

		spk::Vector2Int topLeftCorner = 0;
		spk::Vector2Int downRightCorner = 0;

		for (size_t i = 0; i < p_text.size(); i++)
		{
			const spk::Font::Glyph& glyph = p_fontAtlas->glyph(p_text[i]);

			if (downRightCorner.y < glyph.positions[3].y)
				downRightCorner.y = glyph.positions[3].y;

			if (i == 0)
				topLeftCorner.x = glyph.positions[0].x;

			if (topLeftCorner.y > glyph.positions[0].y)
				topLeftCorner.y = glyph.positions[0].y;

			if (i != p_text.size() - 1)
				downRightCorner.x += glyph.step.x;
			else
				downRightCorner.x += glyph.size.x;

			result.glyphs.push_back(&glyph);
		}

		result.anchorOffset = -topLeftCorner;
		result.size = downRightCorner - topLeftCorner;

		return (result);
	}

	spk::Vector2Int FontRenderer::_computeBaseAnchor(const RenderingData& p_renderingData)
	{
		spk::Vector2Int result = 0;

		switch (_horizontalAlignment)
		{
		case spk::HorizontalAlignment::Left:
		{
			result.x = 0;
			break;
		}

		case spk::HorizontalAlignment::Centered:
		{
			result.x = (_size.x - p_renderingData.size.x) / 2;
			break;
		}

		case spk::HorizontalAlignment::Right:
		default:
		{
			result.x = _size.x - p_renderingData.size.x;
			break;
		}
		}

		switch (_verticalAlignment)
		{
		case spk::VerticalAlignment::Top:
		{
			result.y = 0;
			break;
		}

		case spk::VerticalAlignment::Centered:
		{
			result.y = (_size.y - p_renderingData.size.y) / 2;
			break;
		}

		case spk::VerticalAlignment::Down:
		default:
		{
			result.y = _size.y - p_renderingData.size.y;
			break;
		}
		}

		return (_anchor + result + p_renderingData.anchorOffset);
	}

	void FontRenderer::_updateGPUData()
	{
		_fontAtlas = &(_font->atlas(_textSize, _outlineSize));

		spk::Vector2 deltas[4] = {
			spk::Vector2(0, 0),
			spk::Vector2(0, 1),
			spk::Vector2(1, 0),
			spk::Vector2(1, 1),
		};

		static std::vector<ShaderInput> _bufferShaderInput;
		static std::vector<unsigned int> _bufferRenderingPipelineIndexes;

		_bufferShaderInput.clear();
		_bufferRenderingPipelineIndexes.clear();

		RenderingData renderingData = _computeRenderingData(_fontAtlas, _text);

		spk::Vector2Int glyphAnchor = _computeBaseAnchor(renderingData);

		for (const spk::Font::Glyph* glyph : renderingData.glyphs)
		{
			unsigned int baseIndexes = static_cast<unsigned int>(_bufferShaderInput.size());

			for (size_t i = 0; i < 4; i++)
			{
				ShaderInput newVertex;

				newVertex.position = glyphAnchor + glyph->positions[i];
				newVertex.uvs = glyph->UVs[i];

				_bufferShaderInput.push_back(newVertex);
			}
			glyphAnchor += glyph->step;

			for (size_t i = 0; i < 6; i++)
			{
				_bufferRenderingPipelineIndexes.push_back(baseIndexes + spk::Font::Glyph::indexesOrder[i]);
			}
		}

		_renderingObject.setVertices(_bufferShaderInput);
		_renderingObject.setIndexes(_bufferRenderingPipelineIndexes);
	}
}
