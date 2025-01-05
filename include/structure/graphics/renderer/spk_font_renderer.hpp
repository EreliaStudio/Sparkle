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
        static inline spk::OpenGL::Program* _program;
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

        void _initProgram();
        void _initBuffers();

        void _updateUbo();

    public:
        FontRenderer();

        void setFont(Font* p_font);
        void setFontSize(const Font::Size& p_fontSize);
        void setGlyphColor(const spk::Color& p_color);
        void setOutlineColor(const spk::Color& p_color);

        void clear();

		void prepare(const std::wstring& text, const spk::Vector2Int& anchor, float layer);

        void validate();

        void render();
    };
}
