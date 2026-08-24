#pragma once

#include <GL/glew.h>

#include <cstddef>
#include <memory>

#include "graphics/opengl/gpu_resource.hpp"

namespace spk
{
	class RenderContext;
	class Texture;

	class Sampler final : public GPUResource
	{
	public:
		enum class Filtering
		{
			Nearest,
			Linear
		};

		enum class Wrap
		{
			Repeat,
			MirroredRepeat,
			ClampToEdge,
			ClampToBorder
		};

		enum class MipmapFiltering
		{
			Disabled,
			Nearest,
			Linear
		};

	protected:
		class Instance;

	private:
		std::size_t _bindingPoint = 0;
		const Texture *_texture = nullptr;
		Filtering _filtering = Filtering::Linear;
		Wrap _wrap = Wrap::ClampToEdge;
		MipmapFiltering _mipmapFiltering = MipmapFiltering::Disabled;

		[[nodiscard]] Kind _kind() const noexcept override;
		[[nodiscard]] static GLint _openGLMagFilter(Filtering filtering) noexcept;
		[[nodiscard]] static GLint _openGLMinFilter(Filtering filtering, MipmapFiltering mipmapFiltering) noexcept;
		[[nodiscard]] static GLint _openGLWrap(Wrap wrap) noexcept;

	protected:
		[[nodiscard]] std::unique_ptr<GPUResource::Instance> _create(RenderContext &context) const override;
		void _synchronize(GPUResource::Instance &instance, RenderContext &context) const override;
		void _bind(GPUResource::Instance &instance, RenderContext &context) const override;

	public:
		explicit Sampler(
			std::size_t bindingPoint,
			Filtering filtering = Filtering::Linear,
			Wrap wrap = Wrap::ClampToEdge,
			MipmapFiltering mipmapFiltering = MipmapFiltering::Disabled);

		void setTexture(const Texture *texture) noexcept;
		void setFiltering(Filtering filtering) noexcept;
		void setWrap(Wrap wrap) noexcept;
		void setMipmapFiltering(MipmapFiltering mipmapFiltering) noexcept;
		void setProperties(Filtering filtering, Wrap wrap, MipmapFiltering mipmapFiltering) noexcept;

		[[nodiscard]] std::size_t bindingPoint() const noexcept;
		[[nodiscard]] const Texture *texture() const noexcept;
		[[nodiscard]] Filtering filtering() const noexcept;
		[[nodiscard]] Wrap wrap() const noexcept;
		[[nodiscard]] MipmapFiltering mipmapFiltering() const noexcept;
	};
}
