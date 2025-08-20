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
					vec4 glyphColor;
					vec4 outlineColor;
					float outlineThreshold;
				};

				uniform sampler2D diffuseTexture;

				float computeFormula(float x, float k)
				{
					return (1.0 - (exp(-k * x))) / (1.0 - exp(-k));
				}

				void main()
				{
					float sdf = texture(diffuseTexture, fragmentUVs).r;
    
					if (sdf == 0)
						discard;

					if (sdf < 0.5f)
					{
						outputColor = vec4(outlineColor.rgb, smoothstep(0, 0.5f, sdf));
					}
					else
					{
						outputColor = glyphColor;
					}
				}
				)";

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

		_textInformationsUniformBufferObject = std::move(spk::OpenGL::UniformBufferObject(L"TextInformations", 0, 36));
		_textInformationsUniformBufferObject.addElement(L"glyphColor", 0, sizeof(spk::Color));
		_textInformationsUniformBufferObject.addElement(L"outlineColor", 16, sizeof(spk::Color));
		_textInformationsUniformBufferObject.addElement(L"outlineThreshold", 32, sizeof(float));
	}

	void FontPainter::_updateUniformBufferObject()
	{
		_textInformationsUniformBufferObject[L"glyphColor"] = _glyphColor;
		_textInformationsUniformBufferObject[L"outlineColor"] = _outlineColor;
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
		_textInformationsUniformBufferObject[L"outlineThreshold"] = 0.5f;
		_textInformationsUniformBufferObject.validate();
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
				newVertex.uv = glyph.UVs[i];
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
