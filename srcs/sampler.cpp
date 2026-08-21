#include "sampler.hpp"

#include <stdexcept>

#include "render_context.hpp"
#include "texture.hpp"

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

	GPUResource::Kind Sampler::_kind() const noexcept
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

	std::unique_ptr<GPUResource::Instance> Sampler::_create(RenderContext &) const
	{
		return std::make_unique<Instance>();
	}

	void Sampler::_synchronize(GPUResource::Instance &base, RenderContext &) const
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

	void Sampler::_bind(GPUResource::Instance &base, RenderContext &context) const
	{
		if (_texture == nullptr)
		{
			throw std::logic_error("Cannot activate a Sampler without a Texture");
		}

		_texture->_bindToUnit(_bindingPoint, context);
		glBindSampler(static_cast<GLuint>(_bindingPoint), static_cast<Instance &>(base).identifier);
	}

	Sampler::Sampler(
		std::size_t bindingPoint,
		Filtering filtering,
		Wrap wrap,
		MipmapFiltering mipmapFiltering) :
		_bindingPoint(bindingPoint),
		_texture(nullptr),
		_filtering(filtering),
		_wrap(wrap),
		_mipmapFiltering(mipmapFiltering)
	{
	}

	void Sampler::setTexture(const Texture *texture) noexcept
	{
		_texture = texture;
	}

	void Sampler::setFiltering(Filtering filtering) noexcept
	{
		_filtering = filtering;
	}

	void Sampler::setWrap(Wrap wrap) noexcept
	{
		_wrap = wrap;
	}

	void Sampler::setMipmapFiltering(MipmapFiltering mipmapFiltering) noexcept
	{
		_mipmapFiltering = mipmapFiltering;
	}

	void Sampler::setProperties(Filtering filtering, Wrap wrap, MipmapFiltering mipmapFiltering) noexcept
	{
		_filtering = filtering;
		_wrap = wrap;
		_mipmapFiltering = mipmapFiltering;
	}

	std::size_t Sampler::bindingPoint() const noexcept
	{
		return _bindingPoint;
	}

	const Texture *Sampler::texture() const noexcept
	{
		return _texture;
	}

	Sampler::Filtering Sampler::filtering() const noexcept
	{
		return _filtering;
	}

	Sampler::Wrap Sampler::wrap() const noexcept
	{
		return _wrap;
	}

	Sampler::MipmapFiltering Sampler::mipmapFiltering() const noexcept
	{
		return _mipmapFiltering;
	}
}
