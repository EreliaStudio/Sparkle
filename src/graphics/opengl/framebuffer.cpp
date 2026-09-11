#include "graphics/opengl/framebuffer.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>

#include "core/context/render_context.hpp"

namespace spk
{
	class Framebuffer::Instance final : public GPUResource::Instance
	{
	public:
		GLuint identifier = 0;
		std::size_t colorAttachmentCount = 0;
		GLenum depthAttachmentPoint = GL_NONE;

		Instance()
		{
			glGenFramebuffers(1, &identifier);
			if (identifier == 0)
			{
				throw std::runtime_error("Failed to create OpenGL framebuffer");
			}
		}

		~Instance() override
		{
			if (identifier != 0)
			{
				glDeleteFramebuffers(1, &identifier);
			}
		}
	};

	std::unique_ptr<Texture> Framebuffer::_makeAttachment(
		const Vector2UInt &size,
		Texture::Format format)
	{
		auto result = std::unique_ptr<Texture>(new Texture(Texture::Target::Texture2D));
		result->_allocateRenderTarget(size, format);
		result->validate();
		return result;
	}

	GLenum Framebuffer::_depthAttachmentPoint(Texture::Format format)
	{
		if (Texture::isDepthStencilFormat(format))
		{
			return GL_DEPTH_STENCIL_ATTACHMENT;
		}
		if (Texture::isDepthFormat(format))
		{
			return GL_DEPTH_ATTACHMENT;
		}
		throw std::invalid_argument("Framebuffer depth attachment requires a depth or depth-stencil format");
	}

	void Framebuffer::_validateSize(const Vector2UInt &size)
	{
		if (size.x == 0 || size.y == 0)
		{
			throw std::invalid_argument("Framebuffer size cannot be zero");
		}
		const auto maximum = static_cast<unsigned int>(std::numeric_limits<GLsizei>::max());
		if (size.x > maximum || size.y > maximum)
		{
			throw std::overflow_error("Framebuffer size exceeds the OpenGL GLsizei range");
		}
	}

	GPUResource::Kind Framebuffer::State::_kind() const noexcept
	{
		return GPUResource::Kind::Framebuffer;
	}

	std::unique_ptr<GPUResource::Instance> Framebuffer::State::_create(RenderContext &) const
	{
		return std::make_unique<Instance>();
	}

	void Framebuffer::State::_synchronize(GPUResource::Instance &base, RenderContext &context) const
	{
		auto &instance = static_cast<Instance &>(base);
		glBindFramebuffer(GL_FRAMEBUFFER, instance.identifier);

		GLint maximumColorAttachments = 0;
		GLint maximumDrawBuffers = 0;
		glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maximumColorAttachments);
		glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maximumDrawBuffers);
		const auto maximumSupported = static_cast<std::size_t>(std::min(maximumColorAttachments, maximumDrawBuffers));
		if (_colorAttachments.size() > maximumSupported)
		{
			throw std::out_of_range("Framebuffer color attachment count exceeds the OpenGL limit");
		}

		const std::size_t attachmentSlots = std::max(instance.colorAttachmentCount, _colorAttachments.size());
		std::vector<GLenum> drawBuffers;
		drawBuffers.reserve(_colorAttachments.size());

		for (std::size_t index = 0; index < attachmentSlots; ++index)
		{
			GLuint textureIdentifier = 0;
			if (index < _colorAttachments.size())
			{
				_colorAttachments[index]->activate(context);
				GLint value = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_2D, &value);
				textureIdentifier = static_cast<GLuint>(value);
				drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(index));
			}

			glFramebufferTexture2D(
				GL_FRAMEBUFFER,
				GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(index),
				GL_TEXTURE_2D,
				textureIdentifier,
				0);
		}

		if (instance.depthAttachmentPoint != GL_NONE)
		{
			glFramebufferTexture2D(
				GL_FRAMEBUFFER,
				instance.depthAttachmentPoint,
				GL_TEXTURE_2D,
				0,
				0);
		}

		GLenum depthAttachmentPoint = GL_NONE;
		if (_depthStencilAttachment != nullptr)
		{
			_depthStencilAttachment->activate(context);
			GLint value = 0;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &value);
			depthAttachmentPoint = _depthAttachmentPoint(_depthStencilAttachment->format());
			glFramebufferTexture2D(
				GL_FRAMEBUFFER,
				depthAttachmentPoint,
				GL_TEXTURE_2D,
				static_cast<GLuint>(value),
				0);
		}

		if (drawBuffers.empty())
		{
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
		}
		else
		{
			glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());
			glReadBuffer(drawBuffers.front());
		}

		const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			throw std::runtime_error("OpenGL framebuffer is incomplete (status " + std::to_string(status) + ")");
		}

		instance.colorAttachmentCount = _colorAttachments.size();
		instance.depthAttachmentPoint = depthAttachmentPoint;
	}

	void Framebuffer::State::_bind(GPUResource::Instance &base, RenderContext &) const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, static_cast<Instance &>(base).identifier);
	}

	Framebuffer::Framebuffer(
		const Vector2UInt &size,
		std::vector<Texture::Format> colorFormats,
		std::optional<Texture::Format> depthStencilFormat) :
		GPUResource(std::make_shared<State>())
	{
		auto &content = state<State>();
		content._size = size;
		_validateSize(size);
		if (colorFormats.empty() && depthStencilFormat.has_value() == false)
		{
			throw std::invalid_argument("Framebuffer requires at least one attachment");
		}

		content._colorAttachments.reserve(colorFormats.size());
		for (const Texture::Format format : colorFormats)
		{
			if (Texture::isColorFormat(format) == false)
			{
				throw std::invalid_argument("Framebuffer color attachment requires a color format");
			}
			content._colorAttachments.push_back(_makeAttachment(size, format));
		}

		if (depthStencilFormat.has_value())
		{
			(void)_depthAttachmentPoint(*depthStencilFormat);
			content._depthStencilAttachment = _makeAttachment(size, *depthStencilFormat);
		}

		validate();
	}

	void Framebuffer::resize(const Vector2UInt &size)
	{
		auto &content = state<State>();
		_validateSize(size);
		if (content._size == size)
		{
			return;
		}

		for (auto &attachment : content._colorAttachments)
		{
			attachment->_allocateRenderTarget(size, attachment->format());
			attachment->validate();
		}
		if (content._depthStencilAttachment != nullptr)
		{
			content._depthStencilAttachment->_allocateRenderTarget(size, content._depthStencilAttachment->format());
			content._depthStencilAttachment->validate();
		}

		content._size = size;
		validate();
	}

	const Vector2UInt &Framebuffer::size() const noexcept
	{
		return state<State>()._size;
	}

	std::size_t Framebuffer::colorAttachmentCount() const noexcept
	{
		return state<State>()._colorAttachments.size();
	}

	const Texture &Framebuffer::colorAttachment(std::size_t index) const
	{
		return *state<State>()._colorAttachments.at(index);
	}

	const Texture *Framebuffer::depthStencilAttachment() const noexcept
	{
		return state<State>()._depthStencilAttachment.get();
	}
}
