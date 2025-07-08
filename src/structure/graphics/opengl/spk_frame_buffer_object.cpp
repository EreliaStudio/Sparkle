#include "structure/graphics/opengl/spk_frame_buffer_object.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>

#include <vector>
#include <stdexcept>
#include <cassert>

#include "structure/graphics/opengl/spk_texture_collection.hpp"

namespace spk::OpenGL
{
    static GLenum internalFormatForAttachment(FrameBufferObject::Attachment::Type p_type)
    {
        switch (p_type)
        {
        case FrameBufferObject::Attachment::Type::Color:        return GL_RGBA8;
        case FrameBufferObject::Attachment::Type::Depth:        return GL_DEPTH_COMPONENT24;
        case FrameBufferObject::Attachment::Type::DepthStencil: return GL_DEPTH24_STENCIL8;
        default:                                                return GL_RGBA8;
        }
    }

    static GLenum formatForAttachment(FrameBufferObject::Attachment::Type p_type)
    {
        switch (p_type)
        {
        case FrameBufferObject::Attachment::Type::Color:        return GL_RGBA;
        case FrameBufferObject::Attachment::Type::Depth:        return GL_DEPTH_COMPONENT;
        case FrameBufferObject::Attachment::Type::DepthStencil: return GL_DEPTH_STENCIL;
        default:                                                return GL_RGBA;
        }
    }

    static GLenum typeForAttachment(FrameBufferObject::Attachment::Type p_type)
    {
        switch (p_type)
        {
        case FrameBufferObject::Attachment::Type::Color:        return GL_UNSIGNED_BYTE;
        case FrameBufferObject::Attachment::Type::Depth:        return GL_UNSIGNED_INT;
        case FrameBufferObject::Attachment::Type::DepthStencil: return GL_UNSIGNED_INT_24_8;
        default:                                                return GL_UNSIGNED_BYTE;
        }
    }

    FrameBufferObject::Attachment::Attachment(const std::wstring& p_name, int p_binding, Type p_type) :
        _name(p_name),
        _bindingPoint(p_binding),
        _type(p_type),
        _needsResize(true),
        _cpuTexture(nullptr),
        _gpuTexture(nullptr)
    {
    }

    void FrameBufferObject::Attachment::_allocate(const spk::Vector2UInt& p_size)
    {
        if (p_size.x == 0 || p_size.y == 0)
		{	
            return; 
		}
		
        if (_cpuTexture == nullptr)
        {
            _cpuTexture = spk::SafePointer<spk::Texture>(new spk::Texture());
        }

        _gpuTexture = spk::OpenGL::TextureCollection::textureObject(_cpuTexture);
        if (_gpuTexture == nullptr)
        {
            throw std::runtime_error("Failed to obtain GPU texture object for FBO attachment");
        }
		_size = p_size;

        GLuint texID = _gpuTexture->id();

        glBindTexture(GL_TEXTURE_2D, texID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        GLenum internalFmt = internalFormatForAttachment(_type);
        GLenum format      = formatForAttachment(_type);
        GLenum type        = typeForAttachment(_type);

        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFmt),
                     static_cast<GLsizei>(p_size.x),
                     static_cast<GLsizei>(p_size.y),
                     0, format, type, nullptr);

        glBindTexture(GL_TEXTURE_2D, 0);

        _needsResize = false;
    }

    void FrameBufferObject::Attachment::_resize(const spk::Vector2UInt& p_size)
    {
        if (_gpuTexture == nullptr)
        {
            _allocate(p_size);
            return;
        }

		_size = p_size;

        GLuint texID = _gpuTexture->id();
        glBindTexture(GL_TEXTURE_2D, texID);

        GLenum internalFmt = internalFormatForAttachment(_type);
        GLenum format      = formatForAttachment(_type);
        GLenum type        = typeForAttachment(_type);

        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFmt),
                     static_cast<GLsizei>(p_size.x),
                     static_cast<GLsizei>(p_size.y),
                     0, format, type, nullptr);

        glBindTexture(GL_TEXTURE_2D, 0);

        _needsResize = false;
    }

    void FrameBufferObject::Attachment::_attach(GLuint p_fboID) const
    {
        if (_gpuTexture == nullptr)
		{
            return;
		}

        GLuint texID = _gpuTexture->id();

        GLenum attachmentPoint;
        switch (_type)
        {
        case Type::Color:
            attachmentPoint = GL_COLOR_ATTACHMENT0 + _bindingPoint;
            break;
        case Type::Depth:
            attachmentPoint = GL_DEPTH_ATTACHMENT;
            break;
        case Type::DepthStencil:
            attachmentPoint = GL_DEPTH_STENCIL_ATTACHMENT;
            break;
        default:
            attachmentPoint = GL_COLOR_ATTACHMENT0;
            break;
        }

        glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint, GL_TEXTURE_2D, texID, 0);
    }

    spk::Texture FrameBufferObject::Attachment::save() const
    {
        spk::Texture output;

		if (_gpuTexture == nullptr)
		{
			GENERATE_ERROR("No GPU texture available for FBO attachment");
		}

		const size_t bytesPerPixel = (_type == Type::Color) ? 4 : 4;
		std::vector<uint8_t> pixels(_size.x * _size.y * bytesPerPixel);

		glBindTexture(GL_TEXTURE_2D, _gpuTexture->id());
		glGetTexImage(GL_TEXTURE_2D, 0,
					formatForAttachment(_type),
					typeForAttachment(_type),
					pixels.data());
		glBindTexture(GL_TEXTURE_2D, 0);

		// colour = RGBA, depth = greyscale
		const auto fmt = (_type == Type::Color) ? spk::Texture::Format::RGBA
												: spk::Texture::Format::GreyLevel;

		output.setPixels(pixels, _size, fmt);
		return output;
    }

    FrameBufferObject::FrameBufferObject() :
        _id(0),
        _validated(false),
        _size{0, 0}
    {
    }

    FrameBufferObject::~FrameBufferObject()
    {
        _release();
    }

    FrameBufferObject::FrameBufferObject(FrameBufferObject&& p_other) noexcept :
        _id(p_other._id),
        _validated(p_other._validated),
        _size(p_other._size),
        _attachments(std::move(p_other._attachments))
    {
        p_other._id = 0;
        p_other._validated = false;
        p_other._size = {0, 0};
    }

    FrameBufferObject& FrameBufferObject::operator=(FrameBufferObject&& p_other) noexcept
    {
        if (this != &p_other)
        {
            _release();

            _id = p_other._id;
            _validated = p_other._validated;
            _size = p_other._size;
            _attachments = std::move(p_other._attachments);

            p_other._id = 0;
            p_other._validated = false;
            p_other._size = {0, 0};
        }
        return *this;
    }

    void FrameBufferObject::_allocate()
    {
        if (_id == 0)
        {
            glGenFramebuffers(1, &_id);
        }
    }

    void FrameBufferObject::_release()
    {
        if (_id != 0 && wglGetCurrentContext() != nullptr)
        {
            glDeleteFramebuffers(1, &_id);
        }
        _id = 0;
        _validated = false;
    }

    void FrameBufferObject::addAttachment(const std::wstring& p_attachmentName,
                                          const int&          p_bindingPoint,
                                          const Attachment::Type& p_type)
    {
        _attachments.emplace(p_attachmentName, Attachment(p_attachmentName, p_bindingPoint, p_type));
        _validated = false;
    }

    void FrameBufferObject::resize(const spk::Vector2UInt& p_size)
    {
        _size = p_size;

        for (auto& [name, att] : _attachments)
        {
            att._needsResize = true;
			att._size = p_size;
        }

		_viewport.setWindowSize(p_size);
		_viewport.setGeometry({{0, 0}, p_size});
		_viewport.setClippedGeometry({{0, 0}, p_size});

        _validated = false;
    }

    void FrameBufferObject::_prepareAttachments()
    {
        if (_size.x == 0 || _size.y == 0)
		{
            return;
		}

        for (auto& [name, att] : _attachments)
        {
            if (att._gpuTexture == nullptr)
            {
                att._allocate(_size);
            }
            else if (att._needsResize)
            {
                att._resize(_size);
            }

            att._attach(_id);
        }

        std::vector<GLenum> drawBuffers;
        for (const auto& [name, att] : _attachments)
        {
            if (att._type == Attachment::Type::Color)
            {
                drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + att._bindingPoint);
            }
        }

        if (drawBuffers.empty())
        {
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
        }
        else
        {
            glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());
        }

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        _validated = (status == GL_FRAMEBUFFER_COMPLETE);
        if (!_validated)
        {
            throw std::runtime_error("FBO incomplete after prepareAttachments");
        }
    }

    void FrameBufferObject::activate()
    {
        if (_id == 0)
        {
            _allocate();
        }

        glBindFramebuffer(GL_FRAMEBUFFER, _id);

        if (!_validated)
        {
            _prepareAttachments();
        }

		_lastActiveViewport = spk::Viewport::activeViewport();
		_viewport.apply();
    }

	void FrameBufferObject::clear()
	{
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

    void FrameBufferObject::deactivate()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

		if (_lastActiveViewport != nullptr)
		{
			_lastActiveViewport->apply();
		}
    }

    spk::SafePointer<const FrameBufferObject::Attachment>
    FrameBufferObject::attachment(const std::wstring& p_attachmentName) const
    {
        auto it = _attachments.find(p_attachmentName);
        if (it == _attachments.end())
        {
            return nullptr;
        }
        return spk::SafePointer<const Attachment>(&it->second);
    }
}
