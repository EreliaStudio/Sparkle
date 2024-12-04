#include "structure/graphics/opengl/spk_texture_object.hpp"

namespace spk::OpenGL
{
	void TextureObject::_setupTextureParameters(const Filtering &p_filtering, const Wrap &p_wrap, const Mipmap &p_mipMap)
	{
		switch (p_wrap)
		{
		case spk::Texture::Wrap::Repeat:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			break;
		case spk::Texture::Wrap::ClampToEdge:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			break;
		case spk::Texture::Wrap::ClampToBorder:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			break;
		}

		switch (p_filtering)
		{
		case spk::Texture::Filtering::Nearest:
			glTexParameteri(
				GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (p_mipMap == spk::Texture::Mipmap::Enable) ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			break;
		case spk::Texture::Filtering::Linear:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (p_mipMap == spk::Texture::Mipmap::Enable) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;
		}

		if (p_mipMap == spk::Texture::Mipmap::Enable)
		{
			glGenerateMipmap(GL_TEXTURE_2D);
		}
	}

	TextureObject::TextureObject() :
		_id(0)
	{
		glGenTextures(1, &_id);
	}

	TextureObject::~TextureObject()
	{
		if (_id != 0)
		{
			glDeleteTextures(1, &_id);
			_id = 0;
		}
	}

	TextureObject::TextureObject(TextureObject &&p_other) noexcept :
		_id(std::exchange(p_other._id, 0))
	{
	}

	TextureObject &TextureObject::operator=(TextureObject &&p_other) noexcept
	{
		if (this != &p_other)
		{
			if (_id != 0)
			{
				glDeleteTextures(1, &_id);
			}

			_id = std::exchange(p_other._id, 0);
		}
		return (*this);
	}

	void TextureObject::upload(const spk::SafePointer<const spk::Texture> &p_textureInput)
	{
		updatePixels(p_textureInput);
		updateSettings(p_textureInput);
	}

	void TextureObject::updatePixels(const spk::SafePointer<const spk::Texture> &p_textureInput)
	{
		if (p_textureInput == nullptr)
		{
			return;
		}

		const spk::Texture &texture = *p_textureInput;

		glBindTexture(GL_TEXTURE_2D, _id);

		GLint internalFormat = GL_NONE;
		GLenum externalFormat = GL_NONE;

		OpenGLUtils::convertFormatToGLEnum(texture.format(), internalFormat, externalFormat);

		if (internalFormat == GL_NONE || externalFormat == GL_NONE)
		{
			glBindTexture(GL_TEXTURE_2D, 0);
			return;
		}

		glTexImage2D(
			GL_TEXTURE_2D, 0, internalFormat, texture.size().x, texture.size().y, 0, externalFormat, GL_UNSIGNED_BYTE, texture.pixels().data());

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void TextureObject::updateSettings(const spk::SafePointer<const spk::Texture> &p_textureInput)
	{
		if (p_textureInput == nullptr)
		{
			return;
		}

		const spk::Texture &texture = *p_textureInput;

		glBindTexture(GL_TEXTURE_2D, _id);

		_setupTextureParameters(texture.filtering(), texture.wrap(), texture.mipmap());

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void TextureObject::allocateStorage(
		const spk::Vector2UInt &p_size, const Format &p_format, const Filtering &p_filtering, const Wrap &p_wrap, const Mipmap &p_mipmap)
	{
		glBindTexture(GL_TEXTURE_2D, _id);

		GLint internalFormat;
		GLenum externalFormat;

		OpenGLUtils::convertFormatToGLEnum(p_format, internalFormat, externalFormat);

		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, p_size.x, p_size.y, 0, externalFormat, GL_UNSIGNED_BYTE, nullptr);
		_setupTextureParameters(p_filtering, p_wrap, p_mipmap);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	GLuint TextureObject::id() const
	{
		return (_id);
	}
}
