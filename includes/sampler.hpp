#pragma once

#include <GL/glew.h>

#include <cstddef>
#include <memory>

#include "gpu_resource.hpp"

namespace spk
{
	class RenderContext;
	class Texture;

	class Sampler final : public GPUResource
	{
	protected:
		class Instance;

	private:
		std::size_t _bindingPoint = 0;
		const Texture *_texture = nullptr;

		[[nodiscard]] Kind _kind() const noexcept override;

	protected:
		[[nodiscard]] std::unique_ptr<GPUResource::Instance> _create(RenderContext &context) const override;
		void _synchronize(GPUResource::Instance &instance, RenderContext &context) const override;
		void _bind(GPUResource::Instance &instance, RenderContext &context) const override;

	public:
		explicit Sampler(std::size_t bindingPoint, const Texture *texture = nullptr);

		void setTexture(const Texture *texture) noexcept;

		[[nodiscard]] std::size_t bindingPoint() const noexcept;
		[[nodiscard]] const Texture *texture() const noexcept;
	};
}
