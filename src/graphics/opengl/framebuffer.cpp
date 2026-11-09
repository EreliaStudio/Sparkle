#include "graphics/opengl/framebuffer.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

#include "core/context/render_context.hpp"
#include "exception.hpp"

namespace spk
{
	class Framebuffer::Instance final : public GPUResource::Instance
	{
	public:
		GLuint identifier = 0;
		std::size_t colorAttachmentCount = 0;
		GLenum depthAttachmentPoint = GL_NONE;
		std::optional<Texture::CubeFace> activeFace;

		Instance()
		{
			glGenFramebuffers(1, &identifier);
			if (identifier == 0)
			{
				throw Exception("Failed to create OpenGL framebuffer");
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

	Texture::Handle Framebuffer::_makeAttachment(const Vector2UInt &size, Texture::Format format)
	{
		return Texture::createRenderTarget(Texture::Target::Texture2D, size, format).handle();
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

	void Framebuffer::_validateAttachments() const
	{
		const auto &config = state<State>();
		_validateSize(config._size);
		if (config._colorAttachments.empty() && !config._depthStencilAttachment)
		{
			throw Exception("Framebuffer requires at least one attachment");
		}
		std::unordered_set<GPUResource::Identifier> identities;
		for (const auto &handle : config._colorAttachments)
		{
			if (!handle || !identities.insert(handle.identifier()).second ||
				handle->contentSource() != Texture::ContentSource::RenderTarget ||
				handle->target() != Texture::Target::Texture2D ||
				handle->size() != config._size || !Texture::isColorFormat(handle->format()))
			{
				throw Exception("Framebuffer color attachment has an invalid handle, format, target, size or duplicate role");
			}
		}
		if (const auto &depth = config._depthStencilAttachment; depth)
		{
			if (!*depth || !identities.insert(depth->identifier()).second)
			{
				throw Exception("Framebuffer depth attachment handle is invalid or already attached");
			}
			const auto &value = depth->state();
			const bool invalidCube = value.target() == Texture::Target::TextureCubeMap &&
								 (config._size.x != config._size.y || !Texture::isDepthFormat(value.format()));
			if (value.contentSource() != Texture::ContentSource::RenderTarget ||
				value.size() != config._size ||
				(!Texture::isDepthFormat(value.format()) && !Texture::isDepthStencilFormat(value.format())) ||
				(value.target() != Texture::Target::Texture2D && value.target() != Texture::Target::TextureCubeMap) ||
				invalidCube)
			{
				throw Exception("Framebuffer depth attachment has an incompatible format, target, size or content source");
			}
		}
	}

	void Framebuffer::_refreshViews()
	{
		const auto &config = state<State>();
		std::vector<Texture> colors;
		colors.reserve(config._colorAttachments.size());
		for (const auto &handle : config._colorAttachments)
		{
			colors.push_back(Texture(handle, nullptr));
		}
		std::optional<Texture> depth;
		if (config._depthStencilAttachment)
		{
			depth = Texture(*config._depthStencilAttachment, nullptr);
		}
		_colorViews = std::move(colors);
		_depthView = std::move(depth);
	}

	GPUResource::Kind Framebuffer::State::_kind() const noexcept
	{
		return GPUResource::Kind::Framebuffer;
	}

	bool Framebuffer::State::_hasCubeDepth() const noexcept
	{
		return _depthStencilAttachment &&
			   (*_depthStencilAttachment)->target() == Texture::Target::TextureCubeMap;
	}

	void Framebuffer::State::_validateCapabilities() const
	{
		GLint maxColors = 0, maxDraw = 0, maxTexture = 0;
		glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColors);
		glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDraw);
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexture);
		if (_colorAttachments.size() > static_cast<std::size_t>(std::max(0, std::min(maxColors, maxDraw))))
		{
			throw Exception("Framebuffer color attachments exceed GL_MAX_COLOR_ATTACHMENTS or GL_MAX_DRAW_BUFFERS");
		}
		GLint maxCube = 0;
		if (_hasCubeDepth())
		{
			glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &maxCube);
		}
		const GLint limit = _hasCubeDepth() ? std::min(maxTexture, maxCube) : maxTexture;
		if (_size.x > static_cast<unsigned int>(std::max(0, limit)) ||
			_size.y > static_cast<unsigned int>(std::max(0, limit)))
		{
			throw Exception("Framebuffer dimensions exceed the surface OpenGL texture-size limit");
		}
	}

	void Framebuffer::State::_attachColors(Instance &instance, RenderContext &context) const
	{
		const std::size_t count = std::max(instance.colorAttachmentCount, _colorAttachments.size());
		for (std::size_t index = 0; index < count; ++index)
		{
			GLuint identifier = 0;
			if (index < _colorAttachments.size())
			{
				_colorAttachments[index].activate(context);
				GLint value = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_2D, &value);
				identifier = static_cast<GLuint>(value);
			}
			glFramebufferTexture2D(
				GL_FRAMEBUFFER,
				GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(index),
				GL_TEXTURE_2D,
				identifier,
				0);
		}
		instance.colorAttachmentCount = _colorAttachments.size();
	}

	void Framebuffer::State::_attachDepth(Instance &instance, RenderContext &context) const
	{
		if (instance.depthAttachmentPoint != GL_NONE)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, instance.depthAttachmentPoint, GL_TEXTURE_2D, 0, 0);
		}
		instance.depthAttachmentPoint = GL_NONE;
		instance.activeFace.reset();
		if (!_depthStencilAttachment)
		{
			return;
		}
		const auto &depth = *_depthStencilAttachment;
		depth.activate(context);
		const bool cube = _hasCubeDepth();
		if (cube != context.framebufferCubeFace.has_value())
		{
			throw Exception("Framebuffer cubemap depth requires an explicit face; Texture2D must not specify one");
		}
		GLint identifier = 0;
		glGetIntegerv(cube ? GL_TEXTURE_BINDING_CUBE_MAP : GL_TEXTURE_BINDING_2D, &identifier);
		const auto face = context.framebufferCubeFace.value_or(Texture::CubeFace::PositiveX);
		if (cube && static_cast<unsigned int>(face) >= 6)
		{
			throw Exception("Framebuffer cubemap face is invalid");
		}
		const GLenum target = cube ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(face) : GL_TEXTURE_2D;
		const GLenum point = _depthAttachmentPoint(depth->format());
		glFramebufferTexture2D(GL_FRAMEBUFFER, point, target, static_cast<GLuint>(identifier), 0);
		instance.depthAttachmentPoint = point;
		if (cube)
		{
			instance.activeFace = face;
		}
	}

	void Framebuffer::State::_configureDrawBuffers() const
	{
		if (_colorAttachments.empty())
		{
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
			return;
		}
		std::vector<GLenum> buffers;
		buffers.reserve(_colorAttachments.size());
		for (std::size_t index = 0; index < _colorAttachments.size(); ++index)
		{
			buffers.push_back(GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(index));
		}
		glDrawBuffers(static_cast<GLsizei>(buffers.size()), buffers.data());
		glReadBuffer(buffers.front());
	}

	void Framebuffer::State::_checkComplete() const
	{
		const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			throw Exception("Framebuffer is incomplete on this OpenGL surface (status " + std::to_string(status) + ")");
		}
		if (const GLenum error = glGetError(); error != GL_NO_ERROR)
		{
			throw Exception("Framebuffer attachment operation failed with OpenGL error " + std::to_string(error));
		}
	}

	std::unique_ptr<GPUResource::Instance> Framebuffer::State::_create(RenderContext &) const
	{
		return std::make_unique<Instance>();
	}

	void Framebuffer::State::_synchronize(GPUResource::Instance &base, RenderContext &context) const
	{
		_validateCapabilities();
		auto &instance = static_cast<Instance &>(base);
		glBindFramebuffer(GL_FRAMEBUFFER, instance.identifier);
		_attachColors(instance, context);
		_attachDepth(instance, context);
		_configureDrawBuffers();
		_checkComplete();
	}

	void Framebuffer::State::_bind(GPUResource::Instance &base, RenderContext &context) const
	{
		auto &instance = static_cast<Instance &>(base);
		glBindFramebuffer(GL_FRAMEBUFFER, instance.identifier);
		if (_hasCubeDepth())
		{
			if (!context.framebufferCubeFace)
			{
				throw Exception("Framebuffer cubemap depth cannot be bound without an explicit face");
			}
			if (instance.activeFace != context.framebufferCubeFace)
			{
				_attachDepth(instance, context);
				_checkComplete();
			}
		}
		else if (context.framebufferCubeFace)
		{
			throw Exception("Framebuffer does not have a cubemap depth attachment");
		}
	}

	Framebuffer::Framebuffer() :
		GPUResource(std::shared_ptr<State>(new State()))
	{
	}

	Framebuffer::Framebuffer(
		const Vector2UInt &size,
		std::vector<Texture::Format> colorFormats,
		std::optional<Texture::Format> depthStencilFormat) :
		GPUResource(std::shared_ptr<State>(new State()))
	{
		auto &config = state<State>();
		config._size = size;
		_validateSize(size);
		for (auto format : colorFormats)
		{
			if (!Texture::isColorFormat(format))
			{
				throw std::invalid_argument("Framebuffer color attachment requires a color format");
			}
			config._colorAttachments.push_back(_makeAttachment(size, format));
		}
		if (depthStencilFormat)
		{
			(void)_depthAttachmentPoint(*depthStencilFormat);
			config._depthStencilAttachment = _makeAttachment(size, *depthStencilFormat);
		}
		_validateAttachments();
		_refreshViews();
		validate();
	}

	Framebuffer::Framebuffer(
		const Vector2UInt &size,
		std::vector<Texture::Handle> colorAttachments,
		std::optional<Texture::Handle> depthStencilAttachment) :
		GPUResource(std::shared_ptr<State>(new State()))
	{
		auto &config = state<State>();
		config._size = size;
		config._colorAttachments = std::move(colorAttachments);
		config._depthStencilAttachment = std::move(depthStencilAttachment);
		config._sharedAttachments = true;
		_validateAttachments();
		_refreshViews();
		validate();
	}

	void Framebuffer::activateFace(const Handle &handle, Texture::CubeFace face, RenderContext &context)
	{
		const auto previous = context.framebufferCubeFace;
		context.framebufferCubeFace = face;
		try
		{
			handle.activate(context);
		} catch (...)
		{
			context.framebufferCubeFace = previous;
			throw;
		}
		context.framebufferCubeFace = previous;
	}

	void Framebuffer::resize(const Vector2UInt &size)
	{
		_validateSize(size);
		const auto &config = state<State>();
		if (config._size == size)
		{
			return;
		}
		if (config._sharedAttachments)
		{
			throw Exception("Cannot resize a framebuffer with explicitly shared attachments");
		}
		std::vector<Texture::Format> colors;
		for (const auto &attachment : config._colorAttachments)
		{
			colors.push_back(attachment->format());
		}
		const auto depth = config._depthStencilAttachment
							   ? std::optional<Texture::Format>((*config._depthStencilAttachment)->format())
							   : std::nullopt;
		Framebuffer replacement(size, std::move(colors), depth);
		*this = std::move(replacement);
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
		return _colorViews.at(index);
	}

	const Texture *Framebuffer::depthStencilAttachment() const noexcept
	{
		return _depthView ? std::addressof(*_depthView) : nullptr;
	}

	const Texture::Handle &Framebuffer::colorAttachmentHandle(std::size_t index) const
	{
		return state<State>()._colorAttachments.at(index);
	}

	const std::optional<Texture::Handle> &Framebuffer::depthStencilAttachmentHandle() const noexcept
	{
		return state<State>()._depthStencilAttachment;
	}
}
