#pragma once

#include <filesystem>
#include <map>
#include <unordered_map>
#include <vector>

#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector4.hpp"
#include "structure/graphics/spk_color.hpp"
#include "structure/graphics/opengl/spk_program.hpp"
#include "structure/graphics/opengl/spk_buffer_set.hpp"
#include "structure/graphics/opengl/spk_sampler_object.hpp"
#include "structure/graphics/opengl/spk_uniform_buffer_object.hpp"
#include "structure/graphics/texture/spk_font.hpp"

namespace spk
{
    class FontRenderer
    {
    public:
        struct Vertex
        {
            spk::Vector2 position;
			float layer;
            spk::Vector2 uv;
        };

    private:
        spk::OpenGL::Program _program;
        spk::OpenGL::BufferSet _bufferSet;
        spk::OpenGL::SamplerObject _samplerObject;

        // Uniform buffer object for layer, glyphColor, and outlineColor
        spk::OpenGL::UniformBufferObject _textInformationsUbo;

        Font* _font = nullptr;
		Font::Size _fontSize = {26, 3};
        Font::Atlas* _atlas = nullptr;

        float _layer = 0.0f;
        spk::Color _glyphColor = spk::Color(255, 255, 255, 255);    // white by default
        spk::Color _outlineColor = spk::Color(0, 0, 0, 255);       // black by default

        void _initProgram()
        {
            // Vertex Shader
            const char* vertexShaderSrc = R"(
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
            const char* fragmentShaderSrc = R"(
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

        void _initBuffers()
        {
            _bufferSet = spk::OpenGL::BufferSet({
                {0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2}, // inPosition
                {1, spk::OpenGL::LayoutBufferObject::Attribute::Type::Float},  // inLayer
                {2, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2}  // inUV
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

        void _updateUbo()
        {
            _textInformationsUbo["glyphColor"] = _glyphColor;
            _textInformationsUbo["outlineColor"] = _outlineColor;
            _textInformationsUbo.validate();
        }

    public:
        FontRenderer()
        {
            _initProgram();
            _initBuffers();
            _updateUbo();
        }

        void setFont(Font* p_font)
        {
            _font = p_font;
            _atlas = &_font->atlas(_fontSize);
            _samplerObject.bind(_atlas);
        }

        void setFontSize(const Font::Size& p_fontSize)
        {
            _fontSize = p_fontSize;
            _atlas = &_font->atlas(_fontSize);
            _samplerObject.bind(_atlas);
        }

        void setGlyphColor(const spk::Color& p_color)
        {
            _glyphColor = p_color;
            _updateUbo();
        }

        void setOutlineColor(const spk::Color& p_color)
        {
            _outlineColor = p_color;
            _updateUbo();
        }

        void clear()
        {
            _bufferSet.layout().clear();
            _bufferSet.indexes().clear();
        }

        FontRenderer& operator<<(const Vertex& p_element)
        {
            _bufferSet.layout() << p_element;
            return *this;
        }

        FontRenderer& operator<<(const unsigned int& p_index)
        {
            _bufferSet.indexes() << p_index;
            return *this;
        }

        template<typename Container>
        requires requires (const Container& c) {
            { std::data(c) } -> std::convertible_to<const Vertex*>;
            { std::size(c) } -> std::convertible_to<std::size_t>;
        }
        FontRenderer& operator<<(const Container& p_container)
        {
            _bufferSet.layout() << p_container;
            return *this;
        }

        template<typename Container>
        requires requires (const Container& c) {
            { std::data(c) } -> std::convertible_to<const unsigned int*>;
            { std::size(c) } -> std::convertible_to<std::size_t>;
        }
        FontRenderer& operator<<(const Container& p_container)
        {
            _bufferSet.indexes() << p_container;
            return *this;
        }

        void validate()
        {
            _bufferSet.layout().validate();
            _bufferSet.indexes().validate();
        }

        void render()
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
    };
}
