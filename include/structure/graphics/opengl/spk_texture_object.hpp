#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include <utility>       // std::exchange
#include "structure/spk_safe_pointer.hpp"
#include "structure/graphics/spk_geometry_2D.hpp"
#include "structure/graphics/texture/spk_texture.hpp"
#include "utils/spk_opengl_utils.hpp"

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

        static void _setupTextureParameters(const Filtering& p_filtering, const Wrap& p_wrap, const Mipmap& p_mipMap)
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
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                (p_mipMap == spk::Texture::Mipmap::Enable) ? 
								GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                break;
            case spk::Texture::Filtering::Linear:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                (p_mipMap == spk::Texture::Mipmap::Enable) ?
								GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                break;
            }

			if (p_mipMap == spk::Texture::Mipmap::Enable)
            {
                glGenerateMipmap(GL_TEXTURE_2D);
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

            _setupTextureParameters(texture.filtering(), texture.wrap(), texture.mipmap());

            glBindTexture(GL_TEXTURE_2D, 0);
        }

		void allocateStorage(const spk::Vector2UInt& p_size, 
			const Format& p_format,
			const Filtering& p_filtering,
			const Wrap& p_wrap,
			const Mipmap& p_mipmap)
		{
			glBindTexture(GL_TEXTURE_2D, _id);

            GLint internalFormat;
            GLenum externalFormat, dataType;
            _retrieveOpenGLFormat(p_format, internalFormat, externalFormat, dataType);

            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, p_size.x, p_size.y, 0, externalFormat, dataType, nullptr);
            _setupTextureParameters(p_filtering, p_wrap, p_mipmap);

            glBindTexture(GL_TEXTURE_2D, 0);
		}

        GLuint id() const
        {
            return _id;
        }
    };
}
