#include "texture.hpp"

#include <stdexcept>

#include "render_context.hpp"

namespace spk
{
	class Texture::Instance final : public GPUResource::Instance
	{
	public:
		GLuint identifier = 0;
		GLenum target = 0;

		Instance()
		{
			glGenTextures(1, &identifier);
			if (identifier == 0)
				throw std::runtime_error("Failed to create OpenGL texture");
		}

		~Instance() override
		{
			if (identifier != 0)
				glDeleteTextures(1, &identifier);
		}
	};

	GPUResource::Kind Texture::_kind() const noexcept
	{
		return GPUResource::Kind::Texture;
	}

	GLenum Texture::_openGLTarget(Target target) noexcept
	{
		switch (target)
		{
		case Target::Texture1D: return GL_TEXTURE_1D;
		case Target::Texture2D: return GL_TEXTURE_2D;
		case Target::Texture3D: return GL_TEXTURE_3D;
		case Target::TextureCubeMap: return GL_TEXTURE_CUBE_MAP;
		}

		return GL_TEXTURE_2D;
	}

	void Texture::_bindToUnit(std::size_t bindingPoint, RenderContext &context) const
	{
		glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(bindingPoint));
		activate(context);
	}

	Texture::Texture(Target target) :
		_textureTarget(target)
	{
	}

	std::unique_ptr<GPUResource::Instance> Texture::_create(RenderContext &) const
	{
		return std::make_unique<Instance>();
	}

	void Texture::_synchronize(GPUResource::Instance &base, RenderContext &context) const
	{
		auto &instance = static_cast<Instance &>(base);
		const GLenum target = _openGLTarget(_textureTarget);

		if (instance.target != 0 && instance.target != target)
		{
			glDeleteTextures(1, &instance.identifier);
			glGenTextures(1, &instance.identifier);

			if (instance.identifier == 0)
				throw std::runtime_error("Failed to recreate OpenGL texture");
		}

		instance.target = target;
		glBindTexture(target, instance.identifier);

		_upload(context);
	}

	void Texture::_bind(GPUResource::Instance &base, RenderContext &) const
	{
		auto &instance = static_cast<Instance &>(base);
		glBindTexture(_openGLTarget(_textureTarget), instance.identifier);
	}

	void Texture::_upload(RenderContext &) const
	{
	}

	Texture::Target Texture::target() const noexcept
	{
		return _textureTarget;
	}
}
