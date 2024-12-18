#pragma once

#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"
#include "structure/graphics/spk_color.hpp"
#include "structure/graphics/spk_geometry_2D.hpp"
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

        void _initProgram();
        void _initBuffers();

    public:
        ColorRenderer(const spk::Color& p_color = spk::Color(255, 255, 255));

        void setColor(const spk::Color& p_color);

        void clear();
		
		ColorRenderer& operator<<(const Vertex& p_element);
		ColorRenderer& operator<<(const unsigned int& p_index);

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

		void prepareSquare(const spk::Geometry2D& geom, float layer);

		void validate();

        void render();
    };
}
