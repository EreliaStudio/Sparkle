#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <GL/glew.h>
#include <GL/wglew.h>
#include <gl/GL.h>

#include "math/spk_vector2.hpp"

namespace spk
{
	class Pipeline;

	class Texture
	{
	public:
		enum class Format
		{
			RGB = GL_RGB,
			RGBA = GL_RGBA,
			BGR = GL_BGR,
			BGRA = GL_BGRA,
			GreyLevel = GL_RED,
			Error = GL_NONE
		};

		enum class Filtering
		{
			Nearest = GL_NEAREST,
			Linear = GL_LINEAR
		};

		enum class Wrap
		{
			Repeat = GL_REPEAT,
			ClampToEdge = GL_CLAMP_TO_EDGE,
			ClampToBorder = GL_CLAMP_TO_BORDER
		};

		enum class Mipmap
		{
			Disable,
			Enable
		};
		
	private:
		bool _loaded;
		GLuint _textureID;

		friend class Pipeline;

		void _bind(int p_textureIndex = 0) const;
		void _unbind() const;

	public:
		Texture();

		Texture(const uint8_t* p_textureData, const spk::Vector2UInt& p_textureSize,
			const Format& p_format, const Filtering& p_filtering,
			const Wrap& p_wrap, const Mipmap& p_mipmap);

		virtual ~Texture();

		void uploadToGPU(const uint8_t* p_textureData, const spk::Vector2UInt& p_textureSize,
			const Format& p_format, const Filtering& p_filtering,
			const Wrap& p_wrap, const Mipmap& p_mipmap);

		void releaseGPUMemory();
	};
}
