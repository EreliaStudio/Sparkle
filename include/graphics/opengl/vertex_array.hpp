#pragma once

#include <GL/glew.h>

#include <memory>

#include "graphics/opengl/gpu_resource.hpp"

namespace spk
{
	class IndexBuffer;
	class RenderContext;
	class VertexBuffer;

	class VertexArray final : public GPUResource
	{
	private:
		class Instance;

		const VertexBuffer *_vertexBuffer = nullptr;
		const IndexBuffer *_indexBuffer = nullptr;

		[[nodiscard]] bool _needsConfiguration(const Instance &instance) const noexcept;
		void _disableAttributes(Instance &instance) const;
		void _configureAttributes(Instance &instance) const;
		void _configure(Instance &instance, RenderContext &context) const;

	protected:
		[[nodiscard]] Kind _kind() const noexcept override;
		[[nodiscard]] std::unique_ptr<GPUResource::Instance> _create(RenderContext &context) const override;
		void _synchronize(GPUResource::Instance &instance, RenderContext &context) const override;
		void _bind(GPUResource::Instance &instance, RenderContext &context) const override;

	public:
		VertexArray() = default;

		void setVertexBuffer(const VertexBuffer &vertexBuffer);
		void setIndexBuffer(const IndexBuffer &indexBuffer);
	};
}
