#pragma once

#include <GL/glew.h>

#include <cstddef>
#include <memory>

#include "gpu_resource.hpp"

namespace spk
{
	class RenderContext;
	class Sampler;

	class Texture : public GPUResource
	{
		friend class Sampler;

	public:
		enum class Target
		{
			Texture1D,
			Texture2D,
			Texture3D,
			TextureCubeMap
		};

	protected:
		class Instance;

	private:
		Target _textureTarget = Target::Texture2D;

		[[nodiscard]] Kind _kind() const noexcept override;
		[[nodiscard]] static GLenum _openGLTarget(Target target) noexcept;

		void _bindToUnit(std::size_t bindingPoint, RenderContext &context) const;

	protected:
		explicit Texture(Target target = Target::Texture2D);

		[[nodiscard]] std::unique_ptr<GPUResource::Instance> _create(RenderContext &context) const override;
		void _synchronize(GPUResource::Instance &instance, RenderContext &context) const override;
		void _bind(GPUResource::Instance &instance, RenderContext &context) const override;

		virtual void _upload(RenderContext &context) const;

	public:
		[[nodiscard]] Target target() const noexcept;
	};
}
