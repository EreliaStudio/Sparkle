#include "structure/graphics/renderer/spk_font_renderer.hpp"

#include "structure/graphics/spk_viewport.hpp"

namespace spk
{
	void FontRenderer::_initProgram()
	{
		if (_program == nullptr)
		{
			const char *vertexShaderSrc = R"(
				#version 450

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

			_program = new spk::OpenGL::Program(vertexShaderSrc, fragmentShaderSrc);
		}
	}

	void FontRenderer::_initBuffers()
	{
		_bufferSet = spk::OpenGL::BufferSet({
			{0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2}, // inPosition
			{1, spk::OpenGL::LayoutBufferObject::Attribute::Type::Float},	// inLayer
			{2, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2}	// inUV
		});

		_samplerObject = spk::OpenGL::SamplerObject("diffuseTexture", spk::OpenGL::SamplerObject::Type::Texture2D, 0);

		_textInformationsUbo = std::move(spk::OpenGL::UniformBufferObject(L"TextInformations", 0, 36));
		_textInformationsUbo.addElement(L"glyphColor", 0, sizeof(spk::Color));
		_textInformationsUbo.addElement(L"outlineColor", 16, sizeof(spk::Color));
		_textInformationsUbo.addElement(L"outlineThreshold", 32, sizeof(float));
	}

	void FontRenderer::_updateUbo()
	{
		_textInformationsUbo[L"glyphColor"] = _glyphColor;
		_textInformationsUbo[L"outlineColor"] = _outlineColor;
		_textInformationsUbo.validate();
	}

	FontRenderer::FontRenderer()
	{
		_initProgram();
		_initBuffers();
		_updateUbo();
	}

	void FontRenderer::setFont(const spk::SafePointer<Font>& p_font)
	{
		_font = p_font;
		_atlas = nullptr;
		_samplerObject.bind(nullptr);
	}

	void FontRenderer::setFontSize(const Font::Size &p_fontSize)
	{
		_fontSize = p_fontSize;
		_textInformationsUbo[L"outlineThreshold"] = 0.5f;
		_textInformationsUbo.validate();
		_atlas = nullptr;
		_samplerObject.bind(nullptr);
	}

	void FontRenderer::setGlyphColor(const spk::Color &p_color)
	{
		_glyphColor = p_color;
		_updateUbo();
	}

	void FontRenderer::setOutlineColor(const spk::Color &p_color)
	{
		_outlineColor = p_color;
		_updateUbo();
	}
 
	const spk::SafePointer<spk::Font>& FontRenderer::font() const
	{
		return (_font);
	}
	
	const spk::Font::Size& FontRenderer::fontSize() const
	{
		return (_fontSize);
	}
	
	const spk::Color& FontRenderer::glyphColor() const
	{
		return (_glyphColor);
	}
	
	const spk::Color& FontRenderer::outlineColor() const
	{
		return (_outlineColor);
	}

	void FontRenderer::clear()
	{
		_bufferSet.layout().clear();
		_bufferSet.indexes().clear();
	}

	spk::Vector2Int FontRenderer::computeTextBaselineOffset(const std::wstring& p_text)
	{
		if (_font == nullptr)
		{
			throw std::runtime_error("Font not defined in font renderer");
		}
		return (_font->computeStringBaselineOffset(p_text, _fontSize));
	}

	spk::Vector2UInt FontRenderer::computeTextSize(const std::wstring& p_text)
	{
		if (_font == nullptr)
		{
			throw std::runtime_error("Font not defined in font renderer");
		}
		return (_font->computeStringSize(p_text, _fontSize));
	}
	
	spk::Vector2Int FontRenderer::computeTextAnchor(const spk::Geometry2D& p_geometry, const std::wstring& p_string, spk::HorizontalAlignment p_horizontalAlignment, spk::VerticalAlignment p_verticalAlignment)
	{
		if (_font == nullptr)
		{
			throw std::runtime_error("Font not defined in font renderer");
		}
		return (_font->computeStringAnchor(p_geometry, p_string, _fontSize, p_horizontalAlignment, p_verticalAlignment));
	}

	void FontRenderer::prepare(const std::wstring &text, const spk::Vector2Int &anchor, float layer)
	{
		if (_font == nullptr)
		{
			throw std::runtime_error("Font not defined in font renderer");
		}

		if (_atlas == nullptr)
		{
			_atlas = &_font->atlas(_fontSize);
			_samplerObject.bind(_atlas);
		}
		
		spk::Vector2Int glyphAnchor = anchor;
		unsigned int baseIndexes = 0;

		for (wchar_t c : text)
		{
			const spk::Font::Glyph &glyph = _atlas->glyph(c);

			for (size_t i = 0; i < 4; i++)
			{
				Vertex newVertex;

				Vector3 convertedPoint = spk::Viewport::convertScreenToOpenGL(glyphAnchor + glyph.positions[i], layer);

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

		validate();
	}

	void FontRenderer::validate()
	{
		_bufferSet.layout().validate();
		_bufferSet.indexes().validate();
	}

	void FontRenderer::render()
	{
		if (_atlas == nullptr)
		{
			return;
		}

		_program->activate();
		_bufferSet.activate();
		_samplerObject.activate();
		_textInformationsUbo.activate();

		_program->render(_bufferSet.indexes().nbIndexes(), 1);

		_textInformationsUbo.deactivate();
		_samplerObject.deactivate();
		_bufferSet.deactivate();
		_program->deactivate();
	}
}
