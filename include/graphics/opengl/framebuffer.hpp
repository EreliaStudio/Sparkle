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
			std::vector<Texture::Handle> _colorAttachments;
			std::optional<Texture::Handle> _depthStencilAttachment;
			bool _sharedAttachments = false;

			void _validateCapabilities() const;
			void _attachColors(Instance &instance, RenderContext &context) const;
			void _attachDepth(Instance &instance, RenderContext &context) const;
			void _configureDrawBuffers() const;
			void _checkComplete() const;
			[[nodiscard]] bool _hasCubeDepth() const noexcept;

		protected:
			State() = default;
			State(const State &other) = default;
			[[nodiscard]] std::unique_ptr<GPUResource::State> _clone() const override
			{
				return std::unique_ptr<GPUResource::State>(new State(*this));
			}
			[[nodiscard]] Kind _kind() const noexcept override;
			[[nodiscard]] std::unique_ptr<GPUResource::Instance> _create(RenderContext &context) const override;
			void _synchronize(GPUResource::Instance &instance, RenderContext &context) const override;
			void _bind(GPUResource::Instance &instance, RenderContext &context) const override;
		public:
			[[nodiscard]] const Vector2UInt &size() const noexcept
			{
				return _size;
			}
			[[nodiscard]] const std::vector<Texture::Handle> &colorAttachments() const noexcept
			{
				return _colorAttachments;
			}
			[[nodiscard]] const std::optional<Texture::Handle> &depthStencilAttachment() const noexcept
			{
				return _depthStencilAttachment;
			}
		};
		using Handle = GPUResource::Handle<State>;

	private:
		std::vector<Texture> _colorViews;
		std::optional<Texture> _depthView;

		[[nodiscard]] static Texture::Handle _makeAttachment(const Vector2UInt &size, Texture::Format format);
		[[nodiscard]] static GLenum _depthAttachmentPoint(Texture::Format format);
		static void _validateSize(const Vector2UInt &size);
		void _validateAttachments() const;
		void _refreshViews();

	public:
		Framebuffer();
		explicit Framebuffer(
			const Vector2UInt &size,
			std::vector<Texture::Format> colorFormats = {Texture::Format::RGBA},
			std::optional<Texture::Format> depthStencilFormat = Texture::Format::Depth24Stencil8);
		explicit Framebuffer(
			const Vector2UInt &size,
			std::vector<Texture::Handle> colorAttachments,
			std::optional<Texture::Handle> depthStencilAttachment);

		[[nodiscard]] std::unique_ptr<GPUResource> clone() const override
		{
			auto result = std::make_unique<Framebuffer>(*this);
			result->_setState(_cloneState());
			return result;
		}

		Framebuffer(const Framebuffer &) = default;
		Framebuffer(Framebuffer &&) noexcept = default;
		Framebuffer &operator=(const Framebuffer &) = default;
		Framebuffer &operator=(Framebuffer &&) noexcept = default;

		[[nodiscard]] Handle handle() const
		{
			return createHandle<State>();
		}
		static void activateFace(const Handle &handle, Texture::CubeFace face, RenderContext &context);
		void resize(const Vector2UInt &size);

		[[nodiscard]] const Vector2UInt &size() const noexcept;
		[[nodiscard]] std::size_t colorAttachmentCount() const noexcept;
		[[nodiscard]] const Texture &colorAttachment(std::size_t index) const;
		[[nodiscard]] const Texture *depthStencilAttachment() const noexcept;
		[[nodiscard]] const Texture::Handle &colorAttachmentHandle(std::size_t index) const;
		[[nodiscard]] const std::optional<Texture::Handle> &depthStencilAttachmentHandle() const noexcept;
	};
}
