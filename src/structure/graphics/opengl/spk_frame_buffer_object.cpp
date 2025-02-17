#include "structure/graphics/opengl/spk_frame_buffer_object.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external_libraries/stb_image_write.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <windows.h>
#include <stdexcept>

#include "utils/spk_string_utils.hpp"

#include "structure/spk_iostream.hpp"

#include "spk_debug_macro.hpp"

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
	FrameBufferObject::FrameBufferObject()
	{
		_viewport.invertYAxis();
	}

    FrameBufferObject::FrameBufferObject(const spk::Vector2UInt& p_size) :
		FrameBufferObject()
    {
		_size = p_size;
		resize(_size);
    }

    void FrameBufferObject::addAttachment(const std::wstring& p_name, int p_binding, Type p_type)
    {
        Attachment attachment;
        attachment.binding = p_binding;
        attachment.type = p_type;
        _attachments.emplace(p_name, std::move(attachment));
    }

    void FrameBufferObject::_load()
    {
        for (auto& [name, attachment] : _attachments)
        {
            if (attachment.textureObject._id != 0)
            {
                glDeleteTextures(1, &attachment.textureObject._id);
                attachment.textureObject._id = 0;
            }
        }


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
            texture._filtering = TextureObject::Filtering::Nearest;
            texture._wrap = TextureObject::Wrap::ClampToEdge;
            texture._mipmap = TextureObject::Mipmap::Enable;
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

		if (_depthBufferID == 0)
		{
			glGenRenderbuffers(1, &_depthBufferID);
		}

		glBindRenderbuffer(GL_RENDERBUFFER, _depthBufferID);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, _size.x, _size.y);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthBufferID);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

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

		_needReload = false;
    }

    FrameBufferObject::~FrameBufferObject()
	{
		_releaseResources();
	}

    void FrameBufferObject::_releaseResources()
    {
        if (wglGetCurrentContext() != nullptr)
        {
            if (_framebufferID != 0)
            {
                glDeleteFramebuffers(1, &_framebufferID);
            }
			if (_depthBufferID != 0)
			{
				glDeleteRenderbuffers(1, &_depthBufferID);
			}
            _framebufferID = 0;
			_depthBufferID = 0;
		}

        _attachments.clear();
    }

    void FrameBufferObject::resize(const spk::Vector2UInt& p_size)
    {
        if (_size == p_size)
        {
            return;
        }

        _size = p_size;
		_viewport.setWindowSize(p_size);
        _viewport.setGeometry(Geometry2D(0, 0, p_size));
		_needReload = true;
    }

	const spk::Vector2UInt& FrameBufferObject::size() const
	{
		return (_size);
	}

	void FrameBufferObject::GLState::save()
	{
		blendEnabled      = glIsEnabled(GL_BLEND);
		glGetIntegerv(GL_BLEND_SRC_RGB,    &blendSrcRGB);
		glGetIntegerv(GL_BLEND_DST_RGB,    &blendDstRGB);
		glGetIntegerv(GL_BLEND_SRC_ALPHA,  &blendSrcAlpha);
		glGetIntegerv(GL_BLEND_DST_ALPHA,  &blendDstAlpha);

		cullFaceEnabled   = glIsEnabled(GL_CULL_FACE);
		glGetIntegerv(GL_CULL_FACE_MODE, &cullFaceMode);
		glGetIntegerv(GL_FRONT_FACE,      &frontFaceMode);

		depthTestEnabled  = glIsEnabled(GL_DEPTH_TEST);
		glGetBooleanv(GL_DEPTH_WRITEMASK,  &depthMask);
		glGetFloatv(GL_DEPTH_CLEAR_VALUE,  &clearDepth);
		glGetIntegerv(GL_DEPTH_FUNC,       &depthFunc);

		stencilTestEnabled = glIsEnabled(GL_STENCIL_TEST);
		glGetIntegerv(GL_STENCIL_FUNC,   &stencilFunc);
		glGetIntegerv(GL_STENCIL_REF,    &stencilRef);
		glGetIntegerv(GL_STENCIL_VALUE_MASK, reinterpret_cast<GLint*>(&stencilValueMask));
		glGetIntegerv(GL_STENCIL_FAIL,   &stencilFail);
		glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &stencilZFail);
		glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &stencilZPass);
		glGetIntegerv(GL_STENCIL_WRITEMASK, reinterpret_cast<GLint*>(&stencilWriteMask));

		scissorTestEnabled = glIsEnabled(GL_SCISSOR_TEST);
	}

	void FrameBufferObject::GLState::load()
	{
		if (blendEnabled == true)
			glEnable(GL_BLEND);
		else
			glDisable(GL_BLEND);

		glBlendFuncSeparate(
			blendSrcRGB,
			blendDstRGB,
			blendSrcAlpha,
			blendDstAlpha
		);

		if (cullFaceEnabled == true)
			glEnable(GL_CULL_FACE);
		else
			glDisable(GL_CULL_FACE);

		glCullFace(cullFaceMode);
		glFrontFace(frontFaceMode);

		if (depthTestEnabled == true)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);

		glDepthMask(depthMask);
		glClearDepth(clearDepth);
		glDepthFunc(depthFunc);

		if (stencilTestEnabled == true)
			glEnable(GL_STENCIL_TEST);
		else
			glDisable(GL_STENCIL_TEST);

		glStencilFunc(
			stencilFunc,
			stencilRef,
			stencilValueMask
		);
		glStencilOp(
			stencilFail,
			stencilZFail,
			stencilZPass
		);
		glStencilMask(stencilWriteMask);

		if (scissorTestEnabled == true)
			glEnable(GL_SCISSOR_TEST);
		else
			glDisable(GL_SCISSOR_TEST);

	}

    void FrameBufferObject::activate()
    {
		if (_needReload == true || _framebufferID == 0)
        {
            _load();
			_needReload = false;
        }

		_glState.save();

		_previousViewport = Viewport::activeViewport();

        glBindFramebuffer(GL_FRAMEBUFFER, _framebufferID);
		glBindRenderbuffer(GL_RENDERBUFFER, _depthBufferID);

        try
		{
			_viewport.apply();
		}
		catch (...)
		{
			throw std::runtime_error("Error while applying viewport of framebuffer [" + std::to_string(_framebufferID) + "]");
		}

		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClearDepth(1.0f);
		
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_SCISSOR_TEST);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }

    void FrameBufferObject::deactivate()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		_glState.load();

		if (_previousViewport != nullptr)
		{
			try
			{
				_previousViewport->apply();
			}
			catch (...)
			{
				throw std::runtime_error("Error while applying viewport of framebuffer [" + std::to_string(_framebufferID) + "] previous viewport");
			}
		}
    }

    TextureObject* FrameBufferObject::bindedTexture(const std::wstring& p_name)
    {
		if (_needReload == true || _framebufferID == 0)
        {
            _load();
        }
		
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

    TextureObject FrameBufferObject::saveAsTexture(const std::wstring& p_name)
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

	void FrameBufferObject::saveAsPNG(const std::wstring& p_attachmentName, 
                                      const std::string& p_filePath, 
                                      bool flipVertically)
    {
        auto it = _attachments.find(p_attachmentName);
        if (it == _attachments.end())
        {
            std::cerr << "[FrameBufferObject] Attachment \"" 
                      << std::string(p_attachmentName.begin(), p_attachmentName.end()) 
                      << "\" not found.\n";
            return;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, _framebufferID);

        const Attachment& attachment = it->second;
        if (attachment.binding >= 0)
        {
            glReadBuffer(GL_COLOR_ATTACHMENT0 + attachment.binding);
        }
        else
        {
            std::cerr << "[FrameBufferObject] Attachment \""
                      << std::string(p_attachmentName.begin(), p_attachmentName.end())
                      << "\" has invalid color attachment binding.\n";
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return;
        }

        int width  = static_cast<int>(_size.x);
        int height = static_cast<int>(_size.y);
        std::vector<unsigned char> pixels(width * height * 4);

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        if (flipVertically)
        {
            std::vector<unsigned char> tempRow(width * 4);
            for (int y = 0; y < height / 2; ++y)
            {
                int oppositeY = height - 1 - y;
                memcpy(tempRow.data(), 
                       &pixels[y * width * 4], 
                       width * 4);
                memcpy(&pixels[y * width * 4], 
                       &pixels[oppositeY * width * 4], 
                       width * 4);
                memcpy(&pixels[oppositeY * width * 4], 
                       tempRow.data(), 
                       width * 4);
            }
        }

        if (!stbi_write_png(p_filePath.c_str(), width, height, 4, 
                            pixels.data(), width * 4))
        {
            std::cerr << "[FrameBufferObject] Failed to write PNG to \"" 
                      << p_filePath << "\"\n";
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}
