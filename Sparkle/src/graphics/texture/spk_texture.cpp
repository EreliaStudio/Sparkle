#include "graphics/texture/spk_texture.hpp"

namespace spk
{
	Texture::Texture() :
		_loaded(false)
	{

	}

	Texture::Texture(const uint8_t* p_textureData, const spk::Vector2UInt& p_textureSize,
		const Format& p_format, const Filtering& p_filtering,
		const Wrap& p_wrap, const Mipmap& p_mipmap) :
		_loaded(false)
	{
		uploadToGPU(p_textureData, p_textureSize, p_format, p_filtering, p_wrap, p_mipmap);
	}

	Texture::~Texture()
	{
		releaseGPUMemory();
	}

	void Texture::uploadToGPU(const uint8_t* p_textureData, const spk::Vector2UInt& p_textureSize,
		const Format& p_format, const Filtering& p_filtering,
		const Wrap& p_wrap, const Mipmap& p_mipmap)
	{
		if (_loaded == true)
			releaseGPUMemory();

		glGenTextures(1, &_textureID);
		glBindTexture(GL_TEXTURE_2D, _textureID);

		switch (p_format) 
		{
			case Format::GreyLevel:
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				break;

			case Format::RGB:
			case Format::BGR:
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				break;

			case Format::RGBA:
			case Format::BGRA:
			default:
				glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
				break;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLenum>(p_format), p_textureSize.x, p_textureSize.y, 0, static_cast<GLenum>(p_format), GL_UNSIGNED_BYTE, p_textureData);

		GLint glFiltering = static_cast<GLenum>(p_filtering);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFiltering);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFiltering);

		GLint glWrap = static_cast<GLenum>(p_wrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glWrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glWrap);

		if(p_mipmap == Mipmap::Enable)
		{
			glGenerateMipmap(GL_TEXTURE_2D);
		}

		glBindTexture(GL_TEXTURE_2D, 0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		
		_loaded = true;
	}

	void Texture::releaseGPUMemory()
	{
		if (_loaded == true)
		{
			glDeleteTextures(1, &_textureID);
			_loaded = false;
		}
	}

	void Texture::_bind(int p_textureIndex) const
	{  
		glActiveTexture(GL_TEXTURE0 + p_textureIndex);
		glBindTexture(GL_TEXTURE_2D, _textureID);
	}

	void Texture::_unbind() const
	{
		glActiveTexture(0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}