#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include <utility>       // std::exchange
#include "structure/spk_safe_pointer.hpp"
#include "structure/graphics/spk_geometry_2D.hpp"
#include "structure/graphics/texture/spk_texture.hpp"

namespace spk::OpenGL
{
    class TextureObject
    {
        friend class SamplerObject;
        friend class FrameBufferObject;

    private:
        GLuint _id;

        static void _retrieveOpenGLFormat(spk::Texture::Format format,
                                          GLint& internalFormat,
                                          GLenum& externalFormat,
                                          GLenum& dataType)
        {
            dataType = GL_UNSIGNED_BYTE;

            switch (format)
            {
            case spk::Texture::Format::RGB:
                internalFormat = GL_RGB8;
                externalFormat = GL_RGB;
                break;

            case spk::Texture::Format::RGBA:
                internalFormat = GL_RGBA8;
                externalFormat = GL_RGBA;
                break;

            case spk::Texture::Format::GreyLevel:
                internalFormat = GL_R8;
                externalFormat = GL_RED;
                break;

            case spk::Texture::Format::DualChannel:
                internalFormat = GL_RG8;
                externalFormat = GL_RG;
                break;

            default:
                internalFormat = 0;
                externalFormat = 0;
                dataType = 0;
                break;
            }
        }

        static void _setupTextureParameters(const spk::Texture& texture)
        {
            switch (texture.wrap())
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

            switch (texture.filtering())
            {
            case spk::Texture::Filtering::Nearest:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                (texture.mipmap() == spk::Texture::Mipmap::Enable) ? 
								GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                break;
            case spk::Texture::Filtering::Linear:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                (texture.mipmap() == spk::Texture::Mipmap::Enable) ?
								GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                break;
            }
        }

    public:
        TextureObject() :
			_id(0)
        {
            glGenTextures(1, &_id);
        }

        ~TextureObject()
        {
            if (_id != 0)
            {
                glDeleteTextures(1, &_id);
                _id = 0;
            }
        }

        TextureObject(const TextureObject&) = delete;
        TextureObject& operator=(const TextureObject&) = delete;

        TextureObject(TextureObject&& p_other) noexcept
            : _id(std::exchange(p_other._id, 0))
        {
        }

        TextureObject& operator=(TextureObject&& p_other) noexcept
        {
            if (this != &p_other)
            {
                if (_id != 0)
                {
                    glDeleteTextures(1, &_id);
                }

                _id = std::exchange(p_other._id, 0);
            }
            return *this;
        }

        void upload(const spk::SafePointer<const spk::Texture>& p_textureInput)
        {
            updateData(p_textureInput);
            updateSettings(p_textureInput);
        }

		void updateData(const spk::SafePointer<const spk::Texture>& p_textureInput)
		{
			if (p_textureInput == nullptr)
                return;

            const spk::Texture& texture = *p_textureInput;

            glBindTexture(GL_TEXTURE_2D, _id);

            GLint  internalFormat = 0;
            GLenum externalFormat = 0;
            GLenum dataType       = 0;

            _retrieveOpenGLFormat(texture.format(), internalFormat, externalFormat, dataType);

            if (internalFormat == 0 || externalFormat == 0 || dataType == 0)
            {
                glBindTexture(GL_TEXTURE_2D, 0);
                return;
            }

            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         internalFormat,
                         texture.size().x,
                         texture.size().y,
                         0,
                         externalFormat,
                         dataType,
                         texture.data().data());
						 
            glBindTexture(GL_TEXTURE_2D, 0);
		}

		void updateSettings(const spk::SafePointer<const spk::Texture>& p_textureInput)
        {
            if (p_textureInput == nullptr)
                return;

            const spk::Texture& texture = *p_textureInput;

            glBindTexture(GL_TEXTURE_2D, _id);

            _setupTextureParameters(texture);

            if (texture.mipmap() == spk::Texture::Mipmap::Enable)
            {
                glGenerateMipmap(GL_TEXTURE_2D);
            }

            glBindTexture(GL_TEXTURE_2D, 0);
        }

        GLuint id() const
        {
            return _id;
        }
    };
}
