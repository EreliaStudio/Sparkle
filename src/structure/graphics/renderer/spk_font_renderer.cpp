#include "structure/graphics/renderer/spk_font_renderer.hpp"

#include "structure/graphics/spk_viewport.hpp"

namespace spk
{
	void FontRenderer::_initProgram()
	{
		// Vertex Shader
		const char *vertexShaderSrc = R"(
            #version 450

            layout (location = 0) in vec2 inPosition;
            layout (location = 1) in float inLayer;
            layout (location = 2) in vec2 inUV;
            layout (location = 0) out vec2 fragmentUVs;

            layout(std140, binding = 0) uniform TextInformations
            {
                vec4 glyphColor;
                vec4 outlineColor;
            };

            uniform sampler2D diffuseTexture;

            void main()
            {
                gl_Position = vec4(inPosition, inLayer, 1.0f);
                fragmentUVs = inUV;
            }
            )";

		// Fragment Shader
		const char *fragmentShaderSrc = R"(
            #version 450

            layout (location = 0) in vec2 fragmentUVs;
            layout (location = 0) out vec4 outputColor;

            layout(std140, binding = 0) uniform TextInformations
            {
                vec4 glyphColor;
                vec4 outlineColor;
            };

            uniform sampler2D diffuseTexture;

            float computeFormula(float x, float k)
            {
                return (1.0 - (exp(-k * x))) / (1.0 - exp(-k));
            }

            void main()
            {
                float grayscale = texture(diffuseTexture, fragmentUVs).r;

                if (grayscale == 0)
                {
                    discard;
                }

                float outlineToGlyphThreshold = 0.2f;
                if (grayscale <= outlineToGlyphThreshold)
                {
                    outputColor = outlineColor;
                    outputColor.a = computeFormula(smoothstep(0, outlineToGlyphThreshold, grayscale), 20);
                }
                else
                {
                    float t = computeFormula(smoothstep(outlineToGlyphThreshold, 1.0, grayscale), -20);
                    outputColor = mix(outlineColor, glyphColor, t);
                }
            }
            )";

		_program = spk::OpenGL::Program(vertexShaderSrc, fragmentShaderSrc);
	}

	void FontRenderer::_initBuffers()
	{
		_bufferSet = spk::OpenGL::BufferSet({
			{0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2}, // inPosition
			{1, spk::OpenGL::LayoutBufferObject::Attribute::Type::Float},	// inLayer
			{2, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2}	// inUV
		});

		_samplerObject = spk::OpenGL::SamplerObject("diffuseTexture", spk::OpenGL::SamplerObject::Type::Texture2D, 0);

		// The UBO layout:
		// vec4 glyphColor;   // offset 0
		// vec4 outlineColor; // offset 16
		// total size: 32 bytes

		_textInformationsUbo = spk::OpenGL::UniformBufferObject("TextInformations", 0, 32);
		_textInformationsUbo.addElement("glyphColor", 0, sizeof(spk::Color));
		_textInformationsUbo.addElement("outlineColor", 16, sizeof(spk::Color));
	}

	void FontRenderer::_updateUbo()
	{
		_textInformationsUbo["glyphColor"] = _glyphColor;
		_textInformationsUbo["outlineColor"] = _outlineColor;
		_textInformationsUbo.validate();
	}

	FontRenderer::FontRenderer()
	{
		_initProgram();
		_initBuffers();
		_updateUbo();
	}

	void FontRenderer::setFont(Font *p_font)
	{
		_font = p_font;
		_atlas = &_font->atlas(_fontSize);
		_samplerObject.bind(_atlas);
	}

	void FontRenderer::setFontSize(const Font::Size &p_fontSize)
	{
		_fontSize = p_fontSize;
		_atlas = &_font->atlas(_fontSize);
		_samplerObject.bind(_atlas);
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

	void FontRenderer::clear()
	{
		_bufferSet.layout().clear();
		_bufferSet.indexes().clear();
	}

	FontRenderer &FontRenderer::operator<<(const Vertex &p_element)
	{
		_bufferSet.layout() << p_element;
		return *this;
	}

	FontRenderer &FontRenderer::operator<<(const unsigned int &p_index)
	{
		_bufferSet.indexes() << p_index;
		return *this;
	}

	void FontRenderer::prepare(const std::wstring &text, const spk::Vector2Int &anchor, float layer)
	{
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

				*this << newVertex;
			}

			glyphAnchor += glyph.step;

			for (size_t i = 0; i < 6; i++)
			{
				*this << (baseIndexes + spk::Font::Glyph::indexesOrder[i]);
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

		_program.activate();
		_bufferSet.activate();
		_samplerObject.activate();
		_textInformationsUbo.activate();

		_program.render(_bufferSet.indexes().nbIndexes(), 1);

		_textInformationsUbo.deactivate();
		_samplerObject.deactivate();
		_bufferSet.deactivate();
		_program.deactivate();
	}
}