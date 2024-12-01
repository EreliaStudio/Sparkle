#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include <vector>
#include <cstring>
#include "structure/graphics/spk_geometry_2D.hpp"

namespace spk::OpenGL
{
	class TextureObject
	{
		friend class SamplerObject;
		friend class FrameBufferObject;

	public:
		enum class Format
		{
			RGB = GL_RGB,
			RGBA = GL_RGBA,
			BGR = GL_BGR,
			BGRA = GL_BGRA,
			GreyLevel = GL_RED,
			DualChannel = GL_RG,
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
		GLuint _id;
		bool _ownTexture;
		bool _needUpload = false;
		bool _needSetup = false;

		std::vector<uint8_t> _datas;
		spk::Vector2UInt _size;
		Format _format;
		Filtering _filtering;
		Wrap _wrap;
		Mipmap _mipmap;

		void _upload();
		void _setup();
		size_t _getBytesPerPixel(const Format& format) const;

	public:
		TextureObject();
		
		TextureObject(const TextureObject& other);
		TextureObject& operator=(const TextureObject& other);

		TextureObject(TextureObject&& p_other) noexcept;
		TextureObject& operator=(TextureObject&& p_other) noexcept;

		virtual ~TextureObject();

		void setData(const std::vector<uint8_t>& p_data, const spk::Vector2UInt& p_size,
			const Format& p_format, const Filtering& p_filtering,
			const Wrap& p_wrap, const Mipmap& p_mipmap);

		void setData(const std::vector<uint8_t>& p_data, const spk::Vector2UInt& p_size, const Format& p_format);

		void setData(const uint8_t* p_data, const spk::Vector2UInt& p_size,
			const Format& p_format, const Filtering& p_filtering,
			const Wrap& p_wrap, const Mipmap& p_mipmap);

		void setData(const uint8_t* p_data, const spk::Vector2UInt& p_size, const Format& p_format);

		void setProperties(const Filtering& p_filtering, const Wrap& p_wrap, const Mipmap& p_mipmap);

		const std::vector<uint8_t>& data() const;
		spk::Vector2UInt size() const;
		Format format() const;
		Filtering filtering() const;
		Wrap wrap() const;
		Mipmap mipmap() const;
	};
}
