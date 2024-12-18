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

        void _initProgram()
        {
            const char* vertexShaderSrc = R"(#version 450
            layout(location = 0) in vec2 inPosition;
            layout(location = 1) in float inLayer;
            layout(location = 2) in vec2 inUV;

            out vec2 fragUV;

            void main()
            {
                gl_Position = vec4(inPosition, inLayer, 1.0);
                fragUV = inUV;
            }
            )";

            const char* fragmentShaderSrc = R"(#version 450
            in vec2 fragUV;
            layout(location = 0) out vec4 outputColor;

            uniform sampler2D diffuseTexture;

            void main()
            {
                outputColor = texture(diffuseTexture, fragUV);
            }
            )";

            _program = spk::OpenGL::Program(vertexShaderSrc, fragmentShaderSrc);
        }

        void _initBuffers()
        {
            _bufferSet = spk::OpenGL::BufferSet({
                {0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2}, // position
                {1, spk::OpenGL::LayoutBufferObject::Attribute::Type::Float},   // layer
                {2, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2}  // uv
            });

            _samplerObject = spk::OpenGL::SamplerObject("diffuseTexture", spk::OpenGL::SamplerObject::Type::Texture2D, 0);
        }

    public:
        TextureRenderer()
        {
            _initProgram();
            _initBuffers();
        }

        void setTexture(spk::Image* p_image)
        {
            _image = p_image;
            _samplerObject.bind(_image);
        }

        void clear()
        {
            _bufferSet.layout().clear();
            _bufferSet.indexes().clear();
        }

        TextureRenderer& operator<<(const Vertex& p_element)
        {
            _bufferSet.layout() << p_element;
            return *this;
        }
		
		TextureRenderer& operator<<(const unsigned int& p_index)
		{
			_bufferSet.indexes() << p_index;
			return *this;
		}

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

        void validate()
        {
            _bufferSet.layout().validate();
            _bufferSet.indexes().validate();
        }

        void render()
        {
            if (!_image)
                return;

            _program.activate();
            _bufferSet.activate();
            _samplerObject.activate();

            _program.render(_bufferSet.indexes().nbIndexes(), 1);

            _samplerObject.deactivate();
            _bufferSet.deactivate();
            _program.deactivate();
        }
    };
}
