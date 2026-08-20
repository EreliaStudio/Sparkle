#pragma once

#include <GL/glew.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "gpu_resource.hpp"

namespace spk
{
	class RenderContext;

	class BufferGPUResource : public GPUResource
	{
	public:
		enum class Usage
		{
			StaticDraw,
			DynamicDraw,
			StreamDraw
		};

	protected:
		class Instance;

	private:
		class Storage
		{
		private:
			std::vector<std::max_align_t> _storage;
			std::size_t _size = 0;

			[[nodiscard]] static std::size_t _unitCount(std::size_t size) noexcept;

		public:
			void resize(std::size_t size);
			void append(const void *source, std::size_t size);
			void clear();
			[[nodiscard]] std::byte *data() noexcept;
			[[nodiscard]] const std::byte *data() const noexcept;
			[[nodiscard]] std::size_t size() const noexcept;
		};

		Storage _storage;
		Usage _usage = Usage::StaticDraw;

		[[nodiscard]] Kind _kind() const noexcept override;
		void _allocate(Instance &instance) const;

	protected:
		[[nodiscard]] static GLenum _openGLUsage(Usage usage) noexcept;
		[[nodiscard]] static std::size_t _nextCapacity(std::size_t required);
		[[nodiscard]] static GLuint _identifier(GPUResource::Instance &instance) noexcept;

		BufferGPUResource() = default;

		void _append(const void *data, std::size_t size);
		void _write(const void *data, std::size_t size, std::size_t offset = 0);
		void _resize(std::size_t size);
		[[nodiscard]] std::byte *_data() noexcept;
		[[nodiscard]] const std::byte *_data() const noexcept;
		[[nodiscard]] virtual GLenum _target() const noexcept = 0;
		[[nodiscard]] std::unique_ptr<GPUResource::Instance> _create(RenderContext &context) const override;
		void _synchronize(GPUResource::Instance &instance, RenderContext &context) const override;
		void _bind(GPUResource::Instance &instance, RenderContext &context) const override;

	public:
		void clear();
		void setUsage(Usage usage);
		[[nodiscard]] Usage usage() const noexcept;
		[[nodiscard]] std::size_t size() const noexcept;
	};
}