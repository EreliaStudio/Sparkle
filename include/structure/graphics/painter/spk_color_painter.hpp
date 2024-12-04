#pragma once

#include "structure/graphics/opengl/spk_buffer_set.hpp"
#include "structure/graphics/opengl/spk_program.hpp"
#include "structure/graphics/opengl/spk_uniform_buffer_object.hpp"
#include "structure/graphics/spk_color.hpp"
#include "structure/graphics/spk_geometry_2d.hpp"
#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"

namespace spk
{
	class ColorPainter
	{
	public:
		struct Vertex
		{
			spk::Vector2 position;
			float layer;
		};

	private:
		static inline std::unique_ptr<spk::OpenGL::Program> _program = nullptr;
		spk::OpenGL::BufferSet _bufferSet;
		spk::OpenGL::UniformBufferObject _colorUniformBufferObject;

		std::vector<Vertex> _vertices;
		std::vector<unsigned int> _indexes;

		spk::Color _color;

		void _initProgram();
		void _initBuffers();

	public:
		ColorPainter(const spk::Color &p_color = spk::Color(255, 255, 255));

		void setColor(const spk::Color &p_color);

		void clear();

		void prepareSquare(const spk::Geometry2D &p_geometry, float p_layer);

		void validate();

		void render();
	};
}
