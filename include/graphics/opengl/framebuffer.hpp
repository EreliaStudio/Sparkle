#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "graphics/opengl/gpu_resource.hpp"
#include "graphics/opengl/texture.hpp"
#include "math/vector2.hpp"

namespace spk
{
	class RenderContext;

	class Framebuffer final : public GPUResource
	{
	private:
		class Instance;

	public:
		class State final : public GPUResource::State
		{
			friend class Framebuffer;

		private:
			Vector2UInt _size{0, 0};
			std::vector<std::unique_ptr<Texture>> _colorAttachments;
			std::unique_ptr<Texture> _depthStencilAttachment;

		protected:
			State() = default;
			State(const State &other);
			[[nodiscard]] std::unique_ptr<GPUResource::State> _clone() const override
			{
				return std::unique_ptr<GPUResource::State>(new State(*this));
			}
			[[nodiscard]] Kind _kind() const noexcept override;
			[[nodiscard]] std::unique_ptr<GPUResource::Instance> _create(RenderContext &context) const override;
			void _synchronize(GPUResource::Instance &instance, RenderContext &context) const override;
			void _bind(GPUResource::Instance &instance, RenderContext &context) const override;
		};
		using Handle = GPUResource::Handle<State>;

	private:
		[[nodiscard]] static std::unique_ptr<Texture> _makeAttachment(
			const Vector2UInt &size,
			Texture::Format format);
		[[nodiscard]] static GLenum _depthAttachmentPoint(Texture::Format format);
		static void _validateSize(const Vector2UInt &size);

	public:
		Framebuffer();
		explicit Framebuffer(
			const Vector2UInt &size,
			std::vector<Texture::Format> colorFormats = {Texture::Format::RGBA},
			std::optional<Texture::Format> depthStencilFormat = Texture::Format::Depth24Stencil8);
		[[nodiscard]] std::unique_ptr<GPUResource> clone() const override
		{
			auto result = std::make_unique<Framebuffer>(*this);
			result->_setState(_cloneState());
			return result;
		}

		Framebuffer(const Framebuffer &) = default;
		Framebuffer(Framebuffer &&) noexcept = default;

		Framebuffer &operator=(const Framebuffer &) = default;
		Framebuffer &operator=(Framebuffer &&) = delete;
		[[nodiscard]] Handle handle() const
		{
			return createHandle<State>();
		}

		void resize(const Vector2UInt &size);

		[[nodiscard]] const Vector2UInt &size() const noexcept;
		[[nodiscard]] std::size_t colorAttachmentCount() const noexcept;
		[[nodiscard]] const Texture &colorAttachment(std::size_t index) const;
		[[nodiscard]] const Texture *depthStencilAttachment() const noexcept;
	};
}
