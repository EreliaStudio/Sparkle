#pragma once

#include <GL/glew.h>

#include <memory>

#include "graphics/opengl/gpu_resource.hpp"
#include "graphics/opengl/index_buffer.hpp"
#include "graphics/opengl/vertex_buffer.hpp"

namespace spk
{
	class IndexBuffer;
	class RenderContext;
	class VertexBuffer;

	class VertexArray final : public GPUResource
	{
	private:
		class Instance;

	public:
		class State final : public GPUResource::State
		{
			friend class VertexArray;

		private:
			VertexBuffer::Handle _vertexBuffer;
			IndexBuffer::Handle _indexBuffer;

			[[nodiscard]] bool _needsConfiguration(const Instance &instance) const noexcept;
			void _disableAttributes(Instance &instance) const;
			void _configureAttributes(Instance &instance) const;
			void _configure(Instance &instance, RenderContext &context) const;

		protected:
			[[nodiscard]] Kind _kind() const noexcept override;
			[[nodiscard]] std::unique_ptr<GPUResource::Instance> _create(RenderContext &context) const override;
			void _synchronize(GPUResource::Instance &instance, RenderContext &context) const override;
			void _bind(GPUResource::Instance &instance, RenderContext &context) const override;
		};
		using Handle = GPUResource::Handle<State>;

	public:
		VertexArray();
		[[nodiscard]] Handle handle() const
		{
			return createHandle<State>();
		}

		void setVertexBuffer(const VertexBuffer &vertexBuffer);
		void setIndexBuffer(const IndexBuffer &indexBuffer);
	};
}
