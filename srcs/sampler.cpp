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
				throw std::runtime_error("Failed to create OpenGL sampler");
		}

		~Instance() override
		{
			if (identifier != 0)
				glDeleteSamplers(1, &identifier);
		}
	};

	GPUResource::Kind Sampler::_kind() const noexcept
	{
		return GPUResource::Kind::Sampler;
	}

	std::unique_ptr<GPUResource::Instance> Sampler::_create(RenderContext &) const
	{
		return std::make_unique<Instance>();
	}

	void Sampler::_synchronize(GPUResource::Instance &, RenderContext &) const
	{
	}

	void Sampler::_bind(GPUResource::Instance &base, RenderContext &context) const
	{
		if (_texture == nullptr)
			throw std::logic_error("Cannot activate a Sampler without a Texture");

		_texture->_bindToUnit(_bindingPoint, context);

		auto &instance = static_cast<Instance &>(base);
		glBindSampler(static_cast<GLuint>(_bindingPoint), instance.identifier);
	}

	Sampler::Sampler(std::size_t bindingPoint, const Texture *texture) :
		_bindingPoint(bindingPoint),
		_texture(texture)
	{
	}

	void Sampler::setTexture(const Texture *texture) noexcept
	{
		_texture = texture;
	}

	std::size_t Sampler::bindingPoint() const noexcept
	{
		return _bindingPoint;
	}

	const Texture *Sampler::texture() const noexcept
	{
		return _texture;
	}
}
