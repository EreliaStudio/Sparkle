#pragma once

#include "structure/graphics/opengl/spk_buffer_set.hpp"
#include "structure/graphics/opengl/spk_program.hpp"
#include "structure/graphics/opengl/spk_sampler_object.hpp"
#include "structure/graphics/texture/spk_image.hpp"
#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"

namespace spk
{
	class TexturePainter
	{
	private:
		struct Vertex
		{
			spk::Vector2 position;
			float layer;
			spk::Vector2 uv;
		};

		static inline spk::OpenGL::Program *_program = nullptr;
		spk::OpenGL::BufferSet _bufferSet;
		spk::OpenGL::SamplerObject _samplerObject;

		void _initProgram();
		void _initBuffers();

	public:
		TexturePainter();

		void setTexture(spk::SafePointer<const spk::Texture> p_image);
		spk::SafePointer<const spk::Texture> &texture();
		const spk::SafePointer<const spk::Texture> &texture() const;

		void clear();

		void prepare(const spk::Geometry2D &geom, const spk::Image::Section &section, float layer);

		void validate();

		void render();
	};
}
