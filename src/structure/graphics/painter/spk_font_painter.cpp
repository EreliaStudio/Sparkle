#include "structure/graphics/painter/spk_font_painter.hpp"

#include "structure/graphics/spk_viewport.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	void FontPainter::_initProgram()
	{
		if (_program == nullptr)
		{
			const char *vertexShaderSrc = R"(#version 450

				layout (location = 0) in vec2 inPosition;
				layout (location = 1) in float inLayer;
				layout (location = 2) in vec2 inUV;
				layout (location = 0) out vec2 fragmentUVs;

				void main()
				{
					gl_Position = vec4(inPosition, inLayer, 1.0f);
					fragmentUVs = inUV;
				}
				)";

			const char *fragmentShaderSrc = R"(
#version 450

layout (location = 0) in vec2 fragmentUVs;
layout (location = 0) out vec4 outputColor;

layout(std140, binding = 0) uniform TextInformations
{
    vec4 glyphColor;        // RGB = fill color
    vec4 outlineColor;      // RGB = outline color
    float outlineThreshold; // usually 0.5 for glyph edge, 0.0 if outline active
    float glyphAAPixels;    // AA width in px for fill
    float outlineAAPixels;  // AA width in px for outline
};

uniform sampler2D diffuseTexture;

void main()
{
    float sdf = texture(diffuseTexture, fragmentUVs).r;

	if (sdf <= 1e-5)
	{
		discard;
	}

    float fw = max(1e-4, fwidth(sdf));

    float aaFill    = fw * (glyphAAPixels * 0.5);
    float aaOutline = fw * (outlineAAPixels);

    float fill = smoothstep(0.5 - aaFill, 0.5 + aaFill, sdf);

    float outline = smoothstep(outlineThreshold - aaOutline,
                               outlineThreshold + aaOutline,
                               sdf);

    float outlineOnly = max(outline - fill, 0.0);

    float alpha = max(fill, outline);

    if (alpha <= 1e-4)
        discard;

    vec3 color = glyphColor.rgb * fill + outlineColor.rgb * outlineOnly;

    outputColor = vec4(color, alpha);
})";

			_program = std::make_unique<spk::OpenGL::Program>(vertexShaderSrc, fragmentShaderSrc);
		}
	}

	void FontPainter::_initBuffers()
	{
		_bufferSet = spk::OpenGL::BufferSet({
			{0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2}, // inPosition
			{1, spk::OpenGL::LayoutBufferObject::Attribute::Type::Float},	// inLayer
			{2, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2}	// inUV
		});

		_samplerObject = spk::OpenGL::SamplerObject("diffuseTexture", spk::OpenGL::SamplerObject::Type::Texture2D, 0);

		_textInformationsUniformBufferObject = std::move(spk::OpenGL::UniformBufferObject(L"TextInformations", 0, 64));
		_textInformationsUniformBufferObject.addElement(L"glyphColor", 0, sizeof(spk::Color));
		_textInformationsUniformBufferObject.addElement(L"outlineColor", 16, sizeof(spk::Color));
		_textInformationsUniformBufferObject.addElement(L"outlineThreshold", 32, sizeof(float));
		_textInformationsUniformBufferObject.addElement(L"glyphAAPixels", 36, sizeof(float));
		_textInformationsUniformBufferObject.addElement(L"outlineAAPixels", 40, sizeof(float));
	}

	void FontPainter::_updateUniformBufferObject()
	{
		_textInformationsUniformBufferObject[L"glyphColor"] = _glyphColor;
		_textInformationsUniformBufferObject[L"outlineColor"] = _outlineColor;
		_textInformationsUniformBufferObject[L"outlineThreshold"] = _outlineThreshold;
		_textInformationsUniformBufferObject[L"glyphAAPixels"] = static_cast<float>(_glyphAAPixels);
		_textInformationsUniformBufferObject[L"outlineAAPixels"] = static_cast<float>(_outlineAAPixels);
		_textInformationsUniformBufferObject.validate();
	}

	FontPainter::FontPainter()
	{
		_initProgram();
		_initBuffers();
		_updateUniformBufferObject();
	}

	FontPainter::Contract FontPainter::subscribeToFontEdition(const FontPainter::Job &p_job)
	{
		return (_onFontEditionContractProvider.subscribe(p_job));
	}

	void FontPainter::setFont(const spk::SafePointer<Font> &p_font)
	{
		_font = p_font;
		_atlas = nullptr;
		_samplerObject.bind(nullptr);
	}

	void FontPainter::setFontSize(const Font::Size &p_fontSize)
	{
		_fontSize = p_fontSize;

		if (_fontSize.outline > 0)
		{
			_outlineThreshold = 0.0f;
		}
		else
		{
			_outlineThreshold = 0.5f;
		}

		_updateUniformBufferObject();
		_atlas = nullptr;
		_samplerObject.bind(nullptr);
	}

	void FontPainter::setGlyphColor(const spk::Color &p_color)
	{
		_glyphColor = p_color;
		_updateUniformBufferObject();
	}

	void FontPainter::setOutlineColor(const spk::Color &p_color)
	{
		_outlineColor = p_color;
		_updateUniformBufferObject();
	}

	const spk::SafePointer<spk::Font> &FontPainter::font() const
	{
		return (_font);
	}

	const spk::Font::Size &FontPainter::fontSize() const
	{
		return (_fontSize);
	}

	const spk::Color &FontPainter::glyphColor() const
	{
		return (_glyphColor);
	}

	const spk::Color &FontPainter::outlineColor() const
	{
		return (_outlineColor);
	}

	void FontPainter::setOutlineSharpness(size_t p_pixels)
	{
		_outlineAAPixels = p_pixels; // allow 0 for hard edge
		_updateUniformBufferObject();
	}

	size_t FontPainter::outlineSharpness() const
	{
		return (_outlineAAPixels);
	}

	void FontPainter::setGlyphSharpness(size_t p_pixels)
	{
		_glyphAAPixels = p_pixels; // allow 0 for hard edge
		_updateUniformBufferObject();
	}

	size_t FontPainter::glyphSharpness() const
	{
		return (_glyphAAPixels);
	}

	void FontPainter::clear()
	{
		_bufferSet.layout().clear();
		_bufferSet.indexes().clear();
	}

	spk::Vector2Int FontPainter::computeTextBaselineOffset(const std::wstring &p_text) const
	{
		if (_font == nullptr)
		{
			GENERATE_ERROR("Font not defined in font renderer");
		}
		return (_font->computeStringBaselineOffset(p_text, _fontSize));
	}

	spk::Vector2UInt FontPainter::computeTextSize(const std::wstring &p_text) const
	{
		if (_font == nullptr)
		{
			GENERATE_ERROR("Font not defined in font renderer");
		}
		return (_font->computeStringSize(p_text, _fontSize));
	}

	spk::Vector2Int FontPainter::computeTextAnchor(
		const spk::Geometry2D &p_geometry,
		const std::wstring &p_string,
		spk::HorizontalAlignment p_horizontalAlignment,
		spk::VerticalAlignment p_verticalAlignment) const
	{
		if (_font == nullptr)
		{
			GENERATE_ERROR("Font not defined in font renderer");
		}
		return (_font->computeStringAnchor(p_geometry, p_string, _fontSize, p_horizontalAlignment, p_verticalAlignment));
	}

	void FontPainter::_prepareText(const std::wstring &p_text, const spk::Vector2Int &p_anchor, float p_layer)
	{
		spk::Vector2Int glyphAnchor = p_anchor;
		unsigned int baseIndexes = 0;

		for (wchar_t c : p_text)
		{
			const spk::Font::Glyph &glyph = _atlas->glyph(c);

			for (size_t i = 0; i < 4; i++)
			{
				Vertex newVertex;

				Vector3 convertedPoint = spk::Viewport::convertScreenToOpenGL(glyphAnchor + glyph.positions[i], p_layer);

				newVertex.position = convertedPoint.xy();
				newVertex.uv = glyph.uvs[i];
				newVertex.layer = convertedPoint.z;

				_bufferSet.layout() << newVertex;
			}

			glyphAnchor += glyph.step;

			for (size_t i = 0; i < 6; i++)
			{
				_bufferSet.indexes() << (baseIndexes + spk::Font::Glyph::indexesOrder[i]);
			}

			baseIndexes += 4;
		}
	}

	void FontPainter::prepare(const std::wstring &p_text, const spk::Vector2Int &p_anchor, float p_layer)
	{
		if (_font == nullptr)
		{
			GENERATE_ERROR("Font not defined in font renderer");
		}

		if (_atlas == nullptr)
		{
			_atlas = &_font->atlas(_fontSize);
			_samplerObject.bind(_atlas);

			_onAtlasEditionContract = _atlas->subscribe([&]() { _onFontEditionContractProvider.trigger(); });
		}

		_prepareText(p_text, p_anchor, p_layer);
	}

	void FontPainter::validate()
	{
		_bufferSet.layout().validate();
		_bufferSet.indexes().validate();
	}

	void FontPainter::render()
	{
		if (_atlas == nullptr)
		{
			return;
		}

		_program->activate();
		_bufferSet.activate();
		_samplerObject.activate();
		_textInformationsUniformBufferObject.activate();

		_program->render(_bufferSet.indexes().nbIndexes(), 1);

		_textInformationsUniformBufferObject.deactivate();
		_samplerObject.deactivate();
		_bufferSet.deactivate();
		_program->deactivate();
	}
}
