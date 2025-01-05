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
        static inline spk::OpenGL::Program* _program;
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
		
		void prepareSquare(const spk::Geometry2D& geom, float layer);

		void validate();

        void render();
    };
}
