#pragma once

#include <GL/glew.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "container/buffer.hpp"
#include "graphics/opengl/gpu_resource.hpp"

namespace spk
{
	class RenderContext;

	class BufferGPUResource : public GPUResource
	{
	protected:
		class Instance;

	public:
		enum class Usage
		{
			StaticDraw,
			DynamicDraw,
			StreamDraw
		};

	public:
		class State : public GPUResource::State
		{
		private:
			const Buffer *_buffer = nullptr;
			Usage _usage = Usage::StaticDraw;

			[[nodiscard]] Kind _kind() const noexcept override;
			void _allocate(GPUResource::Instance &instance) const;

		protected:
			[[nodiscard]] std::unique_ptr<GPUResource::State> _clone() const override = 0;
			[[nodiscard]] virtual GLenum _target() const noexcept = 0;
			virtual void _validateBuffer(const Buffer &) const
			{
			}
			[[nodiscard]] std::unique_ptr<GPUResource::Instance> _create(RenderContext &context) const override;
			[[nodiscard]] bool _needsSynchronization(const GPUResource::Instance &instance) const override;
			void _synchronize(GPUResource::Instance &instance, RenderContext &context) const override;
			void _bind(GPUResource::Instance &instance, RenderContext &context) const override;

		public:
			[[nodiscard]] Usage usage() const noexcept
			{
				return _usage;
			}
			[[nodiscard]] std::size_t size() const noexcept
			{
				return _buffer != nullptr ? _buffer->size() : 0;
			}

			[[nodiscard]] const Buffer *buffer() const noexcept
			{
				return _buffer;
			}
			[[nodiscard]] std::uint64_t dataVersion() const
			{
				return _buffer != nullptr ? _buffer->version() : 0;
			}

			friend class BufferGPUResource;
		};

	protected:
		[[nodiscard]] static GLenum _openGLUsage(Usage usage) noexcept;
		[[nodiscard]] static std::size_t _nextCapacity(std::size_t required);
		[[nodiscard]] static GLuint _identifier(GPUResource::Instance &instance) noexcept;

		explicit BufferGPUResource(std::shared_ptr<State> state) :
			GPUResource(std::move(state))
		{
		}

		[[nodiscard]] const std::byte *_data() const;

	public:
		// Non-owning: the CPU buffer must outlive this resource and every retained handle.
		// Rebind after replacing the contents of the buffer object by assignment.
		void bind(const Buffer *buffer);
		[[nodiscard]] const Buffer *buffer() const noexcept;
		void setUsage(Usage usage);
		[[nodiscard]] Usage usage() const noexcept;
		[[nodiscard]] std::size_t size() const noexcept;
		std::vector<std::byte> retrieve(RenderContext &context) const;
	};
}
