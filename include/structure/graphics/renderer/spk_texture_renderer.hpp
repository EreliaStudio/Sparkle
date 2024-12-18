#pragma once

#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"
#include "structure/graphics/opengl/spk_program.hpp"
#include "structure/graphics/opengl/spk_buffer_set.hpp"
#include "structure/graphics/opengl/spk_sampler_object.hpp"
#include "structure/graphics/texture/spk_image.hpp"

namespace spk
{
    class TextureRenderer
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

        spk::Image* _image = nullptr;

        void _initProgram();
        void _initBuffers();

    public:
        TextureRenderer();

        void setTexture(spk::Image* p_image);

        void clear();

        TextureRenderer& operator<<(const Vertex& p_element);
		TextureRenderer& operator<<(const unsigned int& p_index);

        template<typename Container>
        requires requires (const Container& c) {
            { std::data(c) } -> std::convertible_to<const Vertex*>;
            { std::size(c) } -> std::convertible_to<std::size_t>;
        }
        TextureRenderer& operator<<(const Container& p_container)
        {
            _bufferSet.layout() << p_container;
            return *this;
        }

        template<typename Container>
        requires requires (const Container& c) {
            { std::data(c) } -> std::convertible_to<const unsigned int*>;
            { std::size(c) } -> std::convertible_to<std::size_t>;
        }
        TextureRenderer& operator<<(const Container& p_container)
        {
            _bufferSet.indexes() << p_container;
            return *this;
        }

		void prepare(const spk::Geometry2D& geom, const spk::Image::Section& section, float layer);

        void validate();

        void render();
    };
}
