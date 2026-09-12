#pragma once

#include <GL/glew.h>

#include <cstddef>
#include <memory>

#include "graphics/opengl/gpu_resource.hpp"
#include "graphics/opengl/texture.hpp"

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

	private:
		class Instance;

	public:
		class State final : public GPUResource::State
		{
			friend class Sampler;

		private:
			std::size_t _bindingPoint = 0;
			Texture::Handle _texture;
			Filtering _filtering = Filtering::Nearest;
			Wrap _wrap = Wrap::ClampToEdge;
			MipmapFiltering _mipmapFiltering = MipmapFiltering::Disabled;

		protected:
			[[nodiscard]] std::unique_ptr<GPUResource::State> _clone() const override
			{
				return std::make_unique<State>(*this);
			}
			[[nodiscard]] Kind _kind() const noexcept override;
			[[nodiscard]] std::unique_ptr<GPUResource::Instance> _create(RenderContext &context) const override;
			void _synchronize(GPUResource::Instance &instance, RenderContext &context) const override;
			void _bind(GPUResource::Instance &instance, RenderContext &context) const override;
		};
		using Handle = GPUResource::Handle<State>;

	private:
		[[nodiscard]] static GLint _openGLMagFilter(Filtering filtering) noexcept;
		[[nodiscard]] static GLint _openGLMinFilter(Filtering filtering, MipmapFiltering mipmapFiltering) noexcept;
		[[nodiscard]] static GLint _openGLWrap(Wrap wrap) noexcept;

	public:
		explicit Sampler(
			std::size_t bindingPoint = 0,
			Filtering filtering = Filtering::Nearest,
			Wrap wrap = Wrap::ClampToEdge,
			MipmapFiltering mipmapFiltering = MipmapFiltering::Disabled);
		[[nodiscard]] std::unique_ptr<GPUResource> clone() const override
		{
			auto result = std::make_unique<Sampler>(*this);
			result->_setState(_cloneState());
			return result;
		}

		[[nodiscard]] Handle handle() const
		{
			return createHandle<State>();
		}
		void setTexture(const Texture *texture) noexcept;
		void setTexture(Texture::Handle texture) noexcept;
		void setFiltering(Filtering filtering) noexcept;
		void setWrap(Wrap wrap) noexcept;
		void setMipmapFiltering(MipmapFiltering mipmapFiltering) noexcept;
		void setProperties(Filtering filtering, Wrap wrap, MipmapFiltering mipmapFiltering) noexcept;

		[[nodiscard]] std::size_t bindingPoint() const noexcept;
		[[nodiscard]] const Texture::Handle &texture() const noexcept;
		[[nodiscard]] Filtering filtering() const noexcept;
		[[nodiscard]] Wrap wrap() const noexcept;
		[[nodiscard]] MipmapFiltering mipmapFiltering() const noexcept;
	};
}
