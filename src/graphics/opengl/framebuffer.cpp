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

	Framebuffer::RenderAttachment::RenderAttachment(
		Texture::Handle texture,
		std::optional<CubeMap::Face> face) :
		_texture(std::move(texture)),
		_face(face)
	{
	}

	Framebuffer::RenderAttachment Framebuffer::RenderAttachment::create2D(
		const Vector2UInt &size,
		Texture::Format format)
	{
		return RenderAttachment(Texture::_createRenderAttachment(Texture::Target::Texture2D, size, format).handle());
	}

	const Texture::Handle &Framebuffer::RenderAttachment::texture() const noexcept
	{
		return _texture;
	}

	std::optional<CubeMap::Face> Framebuffer::RenderAttachment::face() const noexcept
	{
		return _face;
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
		const auto validate = [&](const RenderAttachment &attachment, bool color) {
			const auto &texture = attachment.texture();
			if (!texture || !identities.insert(texture.identifier()).second)
			{
				throw Exception("Framebuffer attachment handle is missing or duplicated");
			}
			const bool cube = texture->target() == Texture::Target::TextureCubeMap;
			const bool targetValid = cube || texture->target() == Texture::Target::Texture2D;
			const bool formatValid = color ? Texture::isColorFormat(texture->format())
										   : Texture::isDepthFormat(texture->format()) ||
												 Texture::isDepthStencilFormat(texture->format());
			const bool faceValid = cube == attachment.face().has_value() &&
								   (!attachment.face() || static_cast<unsigned int>(*attachment.face()) < 6);
			if (texture->contentSource() != Texture::ContentSource::RenderAttachment ||
				texture->size() != config._size || !targetValid || !formatValid || !faceValid)
			{
				throw Exception("Framebuffer attachment has an incompatible format, target, face or size");
			}
		};
		for (const auto &attachment : config._colorAttachments)
		{
			validate(attachment, true);
		}
		if (config._depthStencilAttachment)
		{
			validate(*config._depthStencilAttachment, false);
		}
	}

	GPUResource::Kind Framebuffer::State::_kind() const noexcept
	{
		return GPUResource::Kind::Framebuffer;
	}

	bool Framebuffer::State::_hasCubeAttachment() const noexcept
	{
		const bool cubeColor = std::any_of(
			_colorAttachments.begin(),
			_colorAttachments.end(),
			[](const RenderAttachment &attachment) {
				return attachment.face().has_value();
			});
		return cubeColor || (_depthStencilAttachment && _depthStencilAttachment->face().has_value());
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
		if (_hasCubeAttachment())
		{
			glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &maxCube);
		}
		const GLint limit = _hasCubeAttachment() ? std::min(maxTexture, maxCube) : maxTexture;
		if (_size.x > static_cast<unsigned int>(std::max(0, limit)) ||
			_size.y > static_cast<unsigned int>(std::max(0, limit)))
		{
			throw Exception("Framebuffer dimensions exceed the surface OpenGL texture-size limit");
		}
	}

	void Framebuffer::State::_attach(GLenum point, const RenderAttachment &attachment, RenderContext &context)
	{
		attachment.texture().activate(context);
		GLint identifier = 0;
		const auto face = attachment.face();
		glGetIntegerv(face ? GL_TEXTURE_BINDING_CUBE_MAP : GL_TEXTURE_BINDING_2D, &identifier);
		const GLenum target = face ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(*face) : GL_TEXTURE_2D;
		glFramebufferTexture2D(GL_FRAMEBUFFER, point, target, static_cast<GLuint>(identifier), 0);
	}

	void Framebuffer::State::_attachColors(Instance &instance, RenderContext &context) const
	{
		const std::size_t count = std::max(instance.colorAttachmentCount, _colorAttachments.size());
		for (std::size_t index = 0; index < count; ++index)
		{
			const GLenum point = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(index);
			if (index < _colorAttachments.size())
			{
				_attach(point, _colorAttachments[index], context);
			}
			else
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, point, GL_TEXTURE_2D, 0, 0);
			}
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
		if (!_depthStencilAttachment)
		{
			return;
		}
		const GLenum point = _depthAttachmentPoint(_depthStencilAttachment->texture()->format());
		_attach(point, *_depthStencilAttachment, context);
		instance.depthAttachmentPoint = point;
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

	void Framebuffer::State::_bind(GPUResource::Instance &base, RenderContext &) const
	{
		auto &instance = static_cast<Instance &>(base);
		glBindFramebuffer(GL_FRAMEBUFFER, instance.identifier);
	}

	Framebuffer::Framebuffer() :
		GPUResource(std::shared_ptr<State>(new State()))
	{
	}

	void Framebuffer::_initialize(
		const Vector2UInt &size,
		std::vector<RenderAttachment> colors,
		std::optional<RenderAttachment> depth,
		bool shared)
	{
		auto &config = state<State>();
		config._size = size;
		config._colorAttachments = std::move(colors);
		config._depthStencilAttachment = std::move(depth);
		config._sharedAttachments = shared;
		_validateAttachments();
		validate();
	}

	Framebuffer::Framebuffer(
		const Vector2UInt &size,
		std::vector<Texture::Format> colorFormats,
		std::optional<Texture::Format> depthStencilFormat) :
		Framebuffer()
	{
		_validateSize(size);
		std::vector<RenderAttachment> colors;
		colors.reserve(colorFormats.size());
		for (const auto format : colorFormats)
		{
			if (!Texture::isColorFormat(format))
			{
				throw std::invalid_argument("Framebuffer color attachment requires a color format");
			}
			colors.push_back(RenderAttachment::create2D(size, format));
		}
		std::optional<RenderAttachment> depth;
		if (depthStencilFormat)
		{
			(void)_depthAttachmentPoint(*depthStencilFormat);
			depth = RenderAttachment::create2D(size, *depthStencilFormat);
		}
		_initialize(size, std::move(colors), std::move(depth), false);
	}

	Framebuffer::Framebuffer(
		const Vector2UInt &size,
		std::vector<Texture::Handle> colorAttachments,
		std::optional<Texture::Handle> depthStencilAttachment) :
		Framebuffer()
	{
		std::vector<RenderAttachment> colors;
		colors.reserve(colorAttachments.size());
		for (auto &texture : colorAttachments)
		{
			colors.emplace_back(std::move(texture));
		}
		std::optional<RenderAttachment> depth;
		if (depthStencilAttachment)
		{
			depth.emplace(std::move(*depthStencilAttachment));
		}
		_initialize(size, std::move(colors), std::move(depth), true);
	}

	Framebuffer::Framebuffer(
		const Vector2UInt &size,
		std::vector<RenderAttachment> colorAttachments,
		std::optional<RenderAttachment> depthStencilAttachment) :
		Framebuffer()
	{
		_initialize(size, std::move(colorAttachments), std::move(depthStencilAttachment), true);
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
			colors.push_back(attachment.texture()->format());
		}
		const auto depth = config._depthStencilAttachment
							   ? std::optional<Texture::Format>(config._depthStencilAttachment->texture()->format())
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

	const Texture::Handle &Framebuffer::colorAttachmentHandle(std::size_t index) const
	{
		return state<State>()._colorAttachments.at(index).texture();
	}

	std::optional<Texture::Handle> Framebuffer::depthStencilAttachmentHandle() const noexcept
	{
		const auto &attachment = state<State>()._depthStencilAttachment;
		return attachment ? std::optional<Texture::Handle>(attachment->texture()) : std::nullopt;
	}
}
