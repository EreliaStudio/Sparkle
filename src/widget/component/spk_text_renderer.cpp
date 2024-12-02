#include "widget/component/spk_text_renderer.hpp"

#include "structure/spk_iostream.hpp"

namespace spk::WidgetComponent
{
	std::string TextRenderer::_pipelineCode = R"(## LAYOUTS DEFINITION ##
0 Vector2Int modelPosition
1 Vector2 modelUVs


## FRAMEBUFFER DEFINITION ##
0 Color pixelColor


## CONSTANTS DEFINITION ##
spk_ScreenConstants_Type spk::ScreenConstants 64 64 {
	canvasMVP 0 64 0 64 1 0 {}
}
spk_WidgetConstants_Type spk::WidgetConstants 4 4 {
	layer 0 4 0 4 1 0 {}
}


## ATTRIBUTES DEFINITION ##
textInformations_Type textInformations 36 48 {
	layer 0 4 0 4 1 0 {}
	glyphColor 4 16 16 16 1 0 {
		r 0 4 0 4 1 0 {}
		g 4 4 4 4 1 0 {}
		b 8 4 8 4 1 0 {}
		a 12 4 12 4 1 0 {}
	}
	outlineColor 20 16 32 16 1 0 {
		r 0 4 0 4 1 0 {}
		g 4 4 4 4 1 0 {}
		b 8 4 8 4 1 0 {}
		a 12 4 12 4 1 0 {}
	}
}


## TEXTURES DEFINITION ##
fontTexture Texture_fontTexture


## VERTEX SHADER CODE ##
#version 450

layout (location = 0) in ivec2 modelPosition;
layout (location = 1) in vec2 modelUVs;
layout (location = 0) out flat int out_instanceID;
layout (location = 1) out vec2 fragmentUVs;

layout(constants) uniform spk_ScreenConstants_Type
{
	mat4 canvasMVP;
} spk_ScreenConstants;

layout(constants) uniform spk_WidgetConstants_Type
{
	float layer;
} spk_WidgetConstants;

layout(attributes) uniform textInformations_Type
{
	float layer;
	vec4 glyphColor;
	vec4 outlineColor;
} textInformations;

uniform sampler2D Texture_fontTexture;

void main()
{
	(gl_Position) = ((spk_ScreenConstants.canvasMVP) * (vec4(modelPosition, (spk_WidgetConstants.layer) + (textInformations.layer), 1.0f)));
	(fragmentUVs) = (modelUVs);
	out_instanceID = gl_InstanceID;
}

## FRAGMENT SHADER CODE ##
#version 450

layout (location = 0) in flat int instanceID;
layout (location = 1) in vec2 fragmentUVs;
layout (location = 0) out vec4 pixelColor;

layout(attributes) uniform textInformations_Type
{
	float layer;
	vec4 glyphColor;
	vec4 outlineColor;
} textInformations;

uniform sampler2D Texture_fontTexture;

float computeFormula(float x, float k)
{
	return ((1.0) - (exp((-k) * (x)))) / ((1.0) - (exp(-k)));
}

void main()
{
	float grayscale = texture(Texture_fontTexture, fragmentUVs).r;
	
	if ((grayscale) == (0))
	{
		discard;
	}

	float outlineToGlyphThreshold = 0.2f;
	if ((grayscale) <= (outlineToGlyphThreshold))
	{
		(pixelColor) = (textInformations.outlineColor);
		(pixelColor.a) = (computeFormula(smoothstep(0, outlineToGlyphThreshold, grayscale), 20));
	}
	else
	{
		float t = computeFormula(smoothstep(outlineToGlyphThreshold, 1.0, grayscale), -20);
		(pixelColor) = mix(textInformations.outlineColor, textInformations.glyphColor, t);
	}
}

)";
	spk::Pipeline TextRenderer::_pipeline = spk::Pipeline(_pipelineCode);

	namespace 
	{
		struct RenderingData
		{
			std::vector<const spk::Font::Glyph *> glyphs;
			spk::Vector2Int anchorOffset;
			spk::Vector2Int size;
		
			RenderingData(spk::Font::Atlas& p_fontAtlas, const std::wstring& p_text)
			{
				spk::Vector2Int topLeftCorner = 0;
				spk::Vector2Int downRightCorner = 0;

				for (size_t i = 0; i < p_text.size(); i++)
				{
					const spk::Font::Glyph& glyph = p_fontAtlas.glyph(p_text[i]);

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

					glyphs.push_back(&glyph);
				}

				anchorOffset = -topLeftCorner;
				size = downRightCorner - topLeftCorner;
			}
		};

		spk::Vector2Int computeBaseAnchor(const RenderingData& p_renderingData, const HorizontalAlignment& p_horizontalAlignment, const VerticalAlignment& p_verticalAlignment, const spk::Vector2UInt& p_size)
		{
			spk::Vector2Int result = 0;

			switch (p_horizontalAlignment)
			{
			case spk::HorizontalAlignment::Left:
			{
				result.x = 0;
				break;
			}

			case spk::HorizontalAlignment::Centered:
			{
				result.x = (static_cast<int>(p_size.x) - p_renderingData.size.x) / 2;
				break;
			}

			case spk::HorizontalAlignment::Right:
			default:
			{
				result.x = static_cast<int>(p_size.x) - p_renderingData.size.x;
				break;
			}
			}

			switch (p_verticalAlignment)
			{
			case spk::VerticalAlignment::Top:
			{
				result.y = 0;
				break;
			}

			case spk::VerticalAlignment::Centered:
			{
				result.y = (static_cast<int>(p_size.y) - p_renderingData.size.y) / 2;
				break;
			}

			case spk::VerticalAlignment::Down:
			default:
			{
				result.y = static_cast<int>(p_size.y) - p_renderingData.size.y;
				break;
			}
			}

			return (result + p_renderingData.anchorOffset);
		}
	}

	void TextRenderer::_updateGPUData()
	{
		struct Vertex
		{
			spk::Vector2Int position;
			spk::Vector2 uvs;
		};

		RenderingData renderingData(_font->atlas(_fontSize), _text);

		spk::Vector2Int glyphAnchor = geometry().anchor + computeBaseAnchor(renderingData, _horizontalAlignment, _verticalAlignment, geometry().size);

		_object.layout().clear();
		_object.indexes().clear();

		for (const auto& glyph : renderingData.glyphs)
		{
			unsigned int baseIndexes = static_cast<unsigned int>(_object.layout().size() / sizeof(Vertex));

			for (size_t i = 0; i < 4; i++)
			{
				Vertex newVertex;

				newVertex.position = glyphAnchor + glyph->positions[i];
				newVertex.uvs = glyph->UVs[i];

				_object.layout() << newVertex;
			}
			glyphAnchor += glyph->step;

			for (size_t i = 0; i < 6; i++)
			{
				_object.indexes() << (baseIndexes + spk::Font::Glyph::indexesOrder[i]);
			}
		}

		_object.layout().validate();
		_object.indexes().validate();
	}

	TextRenderer::TextRenderer() :
		_object(_pipeline.createObject()),
		_textInformations(_object.attribute(L"textInformations")),
		_layerElement(_textInformations[L"layer"]),
		_outlineColorElement(_textInformations[L"outlineColor"]),
		_glyphColorElement(_textInformations[L"glyphColor"]),
		_fontTextureSampler2D(_object.sampler2D(L"fontTexture"))
	{
		_layerElement = _layer;
		_outlineColorElement = _outlineColor;
		_glyphColorElement = _glyphColor;

		_textInformations.validate();
	}

	const float& TextRenderer::layer() const
	{
		return (_layer);
	}

	const spk::Geometry2D& TextRenderer::geometry() const
	{
		return (_geometry);
	}

	const spk::SafePointer<spk::Font>& TextRenderer::font() const
	{
		return (_font);
	}

	const std::wstring& TextRenderer::text() const
	{
		return (_text);
	}

	const spk::Font::Size& TextRenderer::fontSize() const
	{
		return (_fontSize);
	}

	const HorizontalAlignment& TextRenderer::horizontalAlignment() const
	{
		return (_horizontalAlignment);
	}

	const VerticalAlignment& TextRenderer::verticalAlignment() const
	{
		return (_verticalAlignment);
	}
	
	const spk::Color& TextRenderer::outlineColor() const
	{
		return (_outlineColor);
	}
	
	const spk::Color& TextRenderer::glyphColor() const
	{
		return (_glyphColor);
	}

	void TextRenderer::setLayer(const float& p_layer)
	{
		_layer = p_layer;

		_layerElement = _layer;

		_textInformations.validate();
	}

	void TextRenderer::setGeometry(const spk::Geometry2D& p_geometry)
	{
		_geometry = p_geometry;
		_needUpdateGPUData = true;
	}

	void TextRenderer::setFont(const spk::SafePointer<spk::Font> p_font)
	{
		_font = p_font;

		if (_font != nullptr)
		{
			_fontTextureSampler2D.bind(&(_font->atlas(_fontSize)));
		}

		_needUpdateGPUData = true;
	}

	void TextRenderer::setFontSize(const spk::Font::Size& p_fontSize)
	{
		_fontSize = p_fontSize;
		if (_font != nullptr)
		{
			_fontTextureSampler2D.bind(&(_font->atlas(_fontSize)));
		}
		_needUpdateGPUData = true;
	}

	void TextRenderer::setColors(const spk::Color& p_outlineColor, const spk::Color& p_glyphColor)
	{
		_outlineColor = p_outlineColor;
		_glyphColor = p_glyphColor;

		_outlineColorElement = _outlineColor;
		_glyphColorElement = _glyphColor;

		_textInformations.validate();
	}

	void TextRenderer::setOutlineColor(const spk::Color& p_outlineColor)
	{
		_outlineColor = p_outlineColor;

		_outlineColorElement = p_outlineColor;

		_textInformations.validate();
	}

	void TextRenderer::setGlyphColor(const spk::Color& p_glyphColor)
	{
		_glyphColor = p_glyphColor;

		_glyphColorElement = p_glyphColor;

		_textInformations.validate();
	}

	void TextRenderer::setText(const std::wstring& p_text)
	{
		_text = p_text;
		_needUpdateGPUData = true;
	}

	void TextRenderer::setTextHorizontalAlignment(const HorizontalAlignment& p_horizontalAlignment)
	{
		_horizontalAlignment = p_horizontalAlignment;
		_needUpdateGPUData = true;
	}

	void TextRenderer::setTextVerticalAlignment(const VerticalAlignment& p_verticalAlignment)
	{
		_verticalAlignment = p_verticalAlignment;
		_needUpdateGPUData = true;
	}

	void TextRenderer::render()
	{
		if (_needUpdateGPUData == true)
		{
			_updateGPUData();
			_needUpdateGPUData = false;
		}
		_object.render();
	}
}