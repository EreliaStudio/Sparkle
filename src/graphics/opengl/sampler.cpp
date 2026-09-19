#include "graphics/opengl/sampler.hpp"

#include <stdexcept>

#include "core/context/render_context.hpp"
#include "graphics/opengl/texture.hpp"

namespace spk
{
	class Sampler::Instance final : public GPUResource::Instance
	{
	public:
		GLuint identifier = 0;

		Instance()
		{
			glGenSamplers(1, &identifier);
			if (identifier == 0)
			{
				throw std::runtime_error("Failed to create OpenGL sampler");
			}
		}

		~Instance() override
		{
			if (identifier != 0)
			{
				glDeleteSamplers(1, &identifier);
			}
		}
	};

	GPUResource::Kind Sampler::State::_kind() const noexcept
	{
		return GPUResource::Kind::Sampler;
	}

	GLint Sampler::_openGLMagFilter(Filtering filtering) noexcept
	{
		return filtering == Filtering::Nearest ? GL_NEAREST : GL_LINEAR;
	}

	GLint Sampler::_openGLMinFilter(Filtering filtering, MipmapFiltering mipmapFiltering) noexcept
	{
		if (mipmapFiltering == MipmapFiltering::Disabled)
		{
			return _openGLMagFilter(filtering);
		}
		if (mipmapFiltering == MipmapFiltering::Nearest)
		{
			return filtering == Filtering::Nearest ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_NEAREST;
		}
		return filtering == Filtering::Nearest ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_LINEAR;
	}

	GLint Sampler::_openGLWrap(Wrap wrap) noexcept
	{
		switch (wrap)
		{
		case Wrap::Repeat:
			return GL_REPEAT;
		case Wrap::MirroredRepeat:
			return GL_MIRRORED_REPEAT;
		case Wrap::ClampToEdge:
			return GL_CLAMP_TO_EDGE;
		case Wrap::ClampToBorder:
			return GL_CLAMP_TO_BORDER;
		}
		return GL_CLAMP_TO_EDGE;
	}

	std::unique_ptr<GPUResource::Instance> Sampler::State::_create(RenderContext &) const
	{
		return std::make_unique<Instance>();
	}

	void Sampler::State::_synchronize(GPUResource::Instance &base, RenderContext &) const
	{
		auto &instance = static_cast<Instance &>(base);

		glSamplerParameteri(instance.identifier, GL_TEXTURE_MAG_FILTER, _openGLMagFilter(_filtering));
		glSamplerParameteri(instance.identifier, GL_TEXTURE_MIN_FILTER, _openGLMinFilter(_filtering, _mipmapFiltering));

		const GLint wrap = _openGLWrap(_wrap);
		glSamplerParameteri(instance.identifier, GL_TEXTURE_WRAP_S, wrap);
		glSamplerParameteri(instance.identifier, GL_TEXTURE_WRAP_T, wrap);
		glSamplerParameteri(instance.identifier, GL_TEXTURE_WRAP_R, wrap);

		if (_wrap == Wrap::ClampToBorder)
		{
			constexpr GLfloat borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
			glSamplerParameterfv(instance.identifier, GL_TEXTURE_BORDER_COLOR, borderColor);
		}
	}

	void Sampler::State::_bind(GPUResource::Instance &base, RenderContext &context) const
	{
		if (!_texture)
		{
			throw std::logic_error("Cannot activate a Sampler without a Texture");
		}

		glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(_bindingPoint));
		_texture.activate(context);
		glBindSampler(static_cast<GLuint>(_bindingPoint), static_cast<Instance &>(base).identifier);
	}

	Sampler::Sampler(
		std::size_t bindingPoint,
		Filtering filtering,
		Wrap wrap,
		MipmapFiltering mipmapFiltering) :
		GPUResource(std::make_shared<State>())
	{
		auto &content = state<State>();
		content._bindingPoint = bindingPoint;
		content._filtering = filtering;
		content._wrap = wrap;
		content._mipmapFiltering = mipmapFiltering;
	}

	void Sampler::setTexture(const Texture *texture) noexcept
	{
		state<State>()._texture = texture ? texture->handle() : Texture::Handle{};
	}

	void Sampler::setTexture(Texture::Handle texture) noexcept
	{
		state<State>()._texture = std::move(texture);
	}

	void Sampler::setFiltering(Filtering filtering) noexcept
	{
		state<State>()._filtering = filtering;
	}

	void Sampler::setWrap(Wrap wrap) noexcept
	{
		state<State>()._wrap = wrap;
	}

	void Sampler::setMipmapFiltering(MipmapFiltering mipmapFiltering) noexcept
	{
		state<State>()._mipmapFiltering = mipmapFiltering;
	}

	void Sampler::setProperties(Filtering filtering, Wrap wrap, MipmapFiltering mipmapFiltering) noexcept
	{
		auto &content = state<State>();
		content._filtering = filtering;
		content._wrap = wrap;
		content._mipmapFiltering = mipmapFiltering;
	}

	std::size_t Sampler::bindingPoint() const noexcept
	{
		return state<State>()._bindingPoint;
	}

	const Texture::Handle &Sampler::texture() const noexcept
	{
		return state<State>()._texture;
	}

	Sampler::Filtering Sampler::filtering() const noexcept
	{
		return state<State>()._filtering;
	}

	Sampler::Wrap Sampler::wrap() const noexcept
	{
		return state<State>()._wrap;
	}

	Sampler::MipmapFiltering Sampler::mipmapFiltering() const noexcept
	{
		return state<State>()._mipmapFiltering;
	}
}
