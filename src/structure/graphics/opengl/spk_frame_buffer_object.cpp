#include "structure/graphics/opengl/spk_frame_buffer_object.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <windows.h>
#include <stdexcept>

#include "utils/spk_string_utils.hpp"

#include "structure/spk_iostream.hpp"

namespace
{
    void _getTextureFormat(spk::OpenGL::FrameBufferObject::Type p_type, GLenum& p_internalFormat, GLenum& p_format, GLenum& p_dataType)
    {
        switch (p_type)
        {
        case spk::OpenGL::FrameBufferObject::Type::Float4:
        {
            p_internalFormat = GL_RGBA32F;
            p_format = GL_RGBA;
            p_dataType = GL_FLOAT;
            break;
        }
        case spk::OpenGL::FrameBufferObject::Type::Float3:
        {
            p_internalFormat = GL_RGB32F;
            p_format = GL_RGB;
            p_dataType = GL_FLOAT;
            break;
        }
        case spk::OpenGL::FrameBufferObject::Type::Float2:
        {
            p_internalFormat = GL_RG32F;
            p_format = GL_RG;
            p_dataType = GL_FLOAT;
            break;
        }
        case spk::OpenGL::FrameBufferObject::Type::Float:
        {
            p_internalFormat = GL_R32F;
            p_format = GL_RED;
            p_dataType = GL_FLOAT;
            break;
        }
        case spk::OpenGL::FrameBufferObject::Type::Int4:
        {
            p_internalFormat = GL_RGBA32I;
            p_format = GL_RGBA_INTEGER;
            p_dataType = GL_INT;
            break;
        }
        case spk::OpenGL::FrameBufferObject::Type::Int3:
        {
            p_internalFormat = GL_RGB32I;
            p_format = GL_RGB_INTEGER;
            p_dataType = GL_INT;
            break;
        }
        case spk::OpenGL::FrameBufferObject::Type::Int2:
        {
            p_internalFormat = GL_RG32I;
            p_format = GL_RG_INTEGER;
            p_dataType = GL_INT;
            break;
        }
        case spk::OpenGL::FrameBufferObject::Type::Int:
        {
            p_internalFormat = GL_R32I;
            p_format = GL_RED_INTEGER;
            p_dataType = GL_INT;
            break;
        }
        case spk::OpenGL::FrameBufferObject::Type::UInt4:
        {
            p_internalFormat = GL_RGBA32UI;
            p_format = GL_RGBA_INTEGER;
            p_dataType = GL_UNSIGNED_INT;
            break;
        }
        case spk::OpenGL::FrameBufferObject::Type::UInt3:
        {
            p_internalFormat = GL_RGB32UI;
            p_format = GL_RGB_INTEGER;
            p_dataType = GL_UNSIGNED_INT;
            break;
        }
        case spk::OpenGL::FrameBufferObject::Type::UInt2:
        {
            p_internalFormat = GL_RG32UI;
            p_format = GL_RG_INTEGER;
            p_dataType = GL_UNSIGNED_INT;
            break;
        }
        case spk::OpenGL::FrameBufferObject::Type::UInt:
        {
            p_internalFormat = GL_R32UI;
            p_format = GL_RED_INTEGER;
            p_dataType = GL_UNSIGNED_INT;
            break;
        }
        default:
        {
            p_internalFormat = GL_R8;
            p_format = GL_RED;
            p_dataType = GL_UNSIGNED_BYTE;
            break;
        }
        }
    }

    spk::OpenGL::TextureObject::Format _mapFormatToTextureObjectFormat(spk::OpenGL::FrameBufferObject::Type p_type)
    {
        switch (p_type)
        {
        case spk::OpenGL::FrameBufferObject::Type::Float4:
        case spk::OpenGL::FrameBufferObject::Type::Int4:
        case spk::OpenGL::FrameBufferObject::Type::UInt4:
        {
            return spk::OpenGL::TextureObject::Format::RGBA;
        }
        case spk::OpenGL::FrameBufferObject::Type::Float3:
        case spk::OpenGL::FrameBufferObject::Type::Int3:
        case spk::OpenGL::FrameBufferObject::Type::UInt3:
        {
            return spk::OpenGL::TextureObject::Format::RGB;
        }
        case spk::OpenGL::FrameBufferObject::Type::Float2:
        case spk::OpenGL::FrameBufferObject::Type::Int2:
        case spk::OpenGL::FrameBufferObject::Type::UInt2:
        {
            return spk::OpenGL::TextureObject::Format::DualChannel;
        }
        case spk::OpenGL::FrameBufferObject::Type::Float:
        case spk::OpenGL::FrameBufferObject::Type::Int:
        case spk::OpenGL::FrameBufferObject::Type::UInt:
        {
            return spk::OpenGL::TextureObject::Format::GreyLevel;
        }
        default:
        {
            return spk::OpenGL::TextureObject::Format::Error;
        }
        }
    }
}

namespace spk::OpenGL
{
    FrameBufferObject::FrameBufferObject(const spk::Vector2UInt& p_size) :
        _size(p_size),
        _viewport(spk::Geometry2D(0, 0, _size))
    {
    }

    inline void FrameBufferObject::addAttachment(const std::wstring& p_name, int p_binding, Type p_type)
    {
        Attachment attachment;
        attachment.binding = p_binding;
        attachment.type = p_type;
        _attachments.emplace(p_name, std::move(attachment));
    }

    inline void FrameBufferObject::_load()
    {
        if (_framebufferID == 0)
        {
            if (wglGetCurrentContext() != nullptr)
            {
                glGenFramebuffers(1, &_framebufferID);
            }
            else
            {
                throw std::runtime_error("No current OpenGL context.");
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, _framebufferID);

        for (auto& [name, attachment] : _attachments)
        {
            TextureObject& texture = attachment.textureObject;
            texture._ownTexture = true;
            texture._size = _size;
            texture._format = _mapFormatToTextureObjectFormat(attachment.type);
            texture._filtering = TextureObject::Filtering::Linear;
            texture._wrap = TextureObject::Wrap::ClampToEdge;
            texture._mipmap = TextureObject::Mipmap::Disable;
            texture._needUpload = false;
            texture._needSetup = false;

            glGenTextures(1, &texture._id);
            glBindTexture(GL_TEXTURE_2D, texture._id);

            GLenum internalFormat, format, dataType;
            _getTextureFormat(attachment.type, internalFormat, format, dataType);

            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, _size.x, _size.y, 0, format, dataType, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(texture._filtering));
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(texture._filtering));
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(texture._wrap));
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(texture._wrap));

            glBindTexture(GL_TEXTURE_2D, 0);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment.binding, GL_TEXTURE_2D, texture._id, 0);
        }

        std::vector<GLenum> drawBuffers;
        for (const auto& [name, attachment] : _attachments)
        {
            drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + attachment.binding);
        }

        glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            throw std::runtime_error("Framebuffer is not complete after load().");
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    inline void FrameBufferObject::_releaseResources()
    {
        if (wglGetCurrentContext() != nullptr)
        {
            if (_framebufferID != 0)
            {
                glDeleteFramebuffers(1, &_framebufferID);
            }
            _framebufferID = 0;
        }

        _attachments.clear();
    }

    inline void FrameBufferObject::resize(const spk::Vector2UInt& p_size)
    {
        if (_size == p_size)
        {
            return;
        }

        _size = p_size;
        _viewport.setGeometry(Geometry2D(0, 0, p_size));

        for (auto& [name, attachment] : _attachments)
        {
            if (attachment.textureObject._id != 0)
            {
                glDeleteTextures(1, &attachment.textureObject._id);
                attachment.textureObject._id = 0;
            }
        }

        _load();
    }

    inline void FrameBufferObject::activate()
    {
        if (_framebufferID == 0)
        {
            _load();
        }

        glBindFramebuffer(GL_FRAMEBUFFER, _framebufferID);
        _viewport.apply();
    }

    inline void FrameBufferObject::deactivate()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    inline TextureObject* FrameBufferObject::bindedTexture(const std::wstring& p_name)
    {
        auto it = _attachments.find(p_name);
        if (it != _attachments.end())
        {
            return &(it->second.textureObject);
        }
        else
        {
            return nullptr;
        }
    }

    inline TextureObject FrameBufferObject::saveAsTexture(const std::wstring& p_name)
    {
        auto it = _attachments.find(p_name);
        if (it == _attachments.end())
        {
            throw std::runtime_error("Attachment not found: " + spk::StringUtils::wstringToString(p_name));
        }

        Attachment& attachment = it->second;

        if (_framebufferID == 0)
        {
            _load();
        }

        glBindFramebuffer(GL_FRAMEBUFFER, _framebufferID);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment.binding, GL_TEXTURE_2D, 0, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        TextureObject savedTexture = std::move(attachment.textureObject);

        TextureObject newTexture;
        newTexture._ownTexture = true;
        newTexture._size = _size;
        newTexture._format = savedTexture._format;
        newTexture._filtering = savedTexture._filtering;
        newTexture._wrap = savedTexture._wrap;
        newTexture._mipmap = savedTexture._mipmap;
        newTexture._needUpload = false;
        newTexture._needSetup = false;

        glGenTextures(1, &newTexture._id);
        glBindTexture(GL_TEXTURE_2D, newTexture._id);

        GLenum internalFormat, format, dataType;
        _getTextureFormat(attachment.type, internalFormat, format, dataType);

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, _size.x, _size.y, 0, format, dataType, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(newTexture._filtering));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(newTexture._filtering));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(newTexture._wrap));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(newTexture._wrap));

        glBindTexture(GL_TEXTURE_2D, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, _framebufferID);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment.binding, GL_TEXTURE_2D, newTexture._id, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        attachment.textureObject = std::move(newTexture);

        return savedTexture;
    }
}
