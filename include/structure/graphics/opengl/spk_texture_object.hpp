#pragma once

#include <GL/glew.h>

#include <GL/gl.h>

#include "structure/graphics/spk_geometry_2D.hpp"
#include "structure/graphics/texture/spk_texture.hpp"
#include "structure/spk_safe_pointer.hpp"
#include "utils/spk_opengl_utils.hpp"
#include <utility> // std::exchange

namespace spk::OpenGL
{
	class TextureObject
	{
		friend class SamplerObject;
		friend class FrameBufferObject;

	public:
		using Format = spk::Texture::Format;
		using Filtering = spk::Texture::Filtering;
		using Wrap = spk::Texture::Wrap;
		using Mipmap = spk::Texture::Mipmap;

	private:
		GLuint _id;

static void _setupTextureParameters(const Filtering &p_filtering, const Wrap &p_wrap, const Mipmap &p_mipMap);

	public:
TextureObject();

~TextureObject();

		TextureObject(const TextureObject &) = delete;
		TextureObject &operator=(const TextureObject &) = delete;

TextureObject(TextureObject &&p_other) noexcept;

TextureObject &operator=(TextureObject &&p_other) noexcept;

void upload(const spk::SafePointer<const spk::Texture> &p_textureInput);

void updatePixels(const spk::SafePointer<const spk::Texture> &p_textureInput);

void updateSettings(const spk::SafePointer<const spk::Texture> &p_textureInput);

void allocateStorage(const spk::Vector2UInt &p_size, const Format &p_format, const Filtering &p_filtering, const Wrap &p_wrap,
 const Mipmap &p_mipmap);

GLuint id() const;
	};
}
