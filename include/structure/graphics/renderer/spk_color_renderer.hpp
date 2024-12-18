#pragma once

#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"
#include "structure/graphics/spk_color.hpp"
#include "structure/graphics/opengl/spk_program.hpp"
#include "structure/graphics/opengl/spk_buffer_set.hpp"
#include "structure/graphics/opengl/spk_uniform_buffer_object.hpp"

namespace spk
{
    class ColorRenderer
    {
    public:
        struct Vertex
        {
            spk::Vector2 position;
            float layer;
        };

    private:
        spk::OpenGL::Program _program;
        spk::OpenGL::BufferSet _bufferSet;
        spk::OpenGL::UniformBufferObject _colorUbo;

        std::vector<Vertex> _vertices;
        std::vector<unsigned int> _indexes;

        spk::Color _color;

        void _initProgram()
        {
            const char* vertexShaderSrc = R"(#version 450
            layout(location = 0) in vec2 inPosition;
            layout(location = 1) in float inLayer;

            layout(std140, binding = 0) uniform ColorData {
                vec4 uColor;
            };

            void main()
            {
                // We can expand this if needed for transformations
                gl_Position = vec4(inPosition, inLayer, 1.0);
            }
            )";

            const char* fragmentShaderSrc = R"(#version 450
            layout(std140, binding = 0) uniform ColorData {
                vec4 uColor;
            };

            layout(location = 0) out vec4 outputColor;

            void main()
            {
                outputColor = uColor;
            }
            )";

            _program = spk::OpenGL::Program(vertexShaderSrc, fragmentShaderSrc);
        }

        void _initBuffers()
        {
            _bufferSet = spk::OpenGL::BufferSet({
                {0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2}, // position
                {1, spk::OpenGL::LayoutBufferObject::Attribute::Type::Float}    // layer
            });

            _colorUbo = spk::OpenGL::UniformBufferObject("ColorData", 0, 16);
            _colorUbo.addElement("uColor", 0, 16);
        }

    public:
        ColorRenderer(const spk::Color& p_color = spk::Color(255, 255, 255))
        {
            _initProgram();
            _initBuffers();
			setColor(p_color);
        }

        void setColor(const spk::Color& p_color)
        {
            _color = p_color;

            _colorUbo["uColor"] = _color;
            _colorUbo.validate();
        }

        void clear()
        {
            _bufferSet.layout().clear();
            _bufferSet.indexes().clear();
        }
		
		ColorRenderer& operator<<(const Vertex& p_element)
		{
			_bufferSet.layout() << p_element;
			return *this;
		}
		
		ColorRenderer& operator<<(const unsigned int& p_index)
		{
			_bufferSet.indexes() << p_index;
			return *this;
		}

		template<typename Container>
		requires  requires (const Container& c) {
			{ std::data(c) } -> std::convertible_to<const Vertex*>;
			{ std::size(c) } -> std::convertible_to<std::size_t>;
		}
		ColorRenderer& operator<<(const Container& p_container)
		{
			_bufferSet.layout() << p_container;
			return *this;
		}

		template<typename Container>
		requires  requires (const Container& c) {
			{ std::data(c) } -> std::convertible_to<const unsigned int*>;
			{ std::size(c) } -> std::convertible_to<std::size_t>;
		}
		ColorRenderer& operator<<(const Container& p_container)
		{
			_bufferSet.indexes() << p_container;
			return *this;
		}

		void prepareSquare(const spk::Geometry2D& geom, float layer)
		{
			size_t nbVertex = _bufferSet.layout().size() / sizeof(Vertex);

			spk::Vector3 topLeft = spk::Viewport::convertScreenToOpenGL({geom.anchor.x, geom.anchor.y}, layer);
			spk::Vector3 bottomRight = spk::Viewport::convertScreenToOpenGL({geom.anchor.x + static_cast<int32_t>(geom.size.x),
																			geom.anchor.y + static_cast<int32_t>(geom.size.y)}, layer);

			*this << Vertex{ { topLeft.x,      bottomRight.y }, topLeft.z }
				  << Vertex{ { bottomRight.x,  bottomRight.y }, topLeft.z }
				  << Vertex{ { topLeft.x,      topLeft.y     }, topLeft.z }
				  << Vertex{ { bottomRight.x,  topLeft.y     }, topLeft.z };

			std::array<unsigned int, 6> indices = {0, 1, 2, 2, 1, 3};
			for (const auto& index : indices)
			{
				*this << index + nbVertex;
			}
		}

		void validate()
		{
            _bufferSet.layout().validate();
            _bufferSet.indexes().validate();
		}

        void render()
        {
            _program.activate();
            _bufferSet.activate();
            _colorUbo.activate();

            _program.render(_bufferSet.indexes().nbIndexes(), 1);

            _colorUbo.deactivate();
            _bufferSet.deactivate();
            _program.deactivate();
        }
    };
}
