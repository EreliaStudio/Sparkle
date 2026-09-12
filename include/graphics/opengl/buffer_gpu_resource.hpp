#pragma once

#include <GL/glew.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

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
			class Storage
			{
			private:
				std::vector<std::max_align_t> _storage;
				std::size_t _size = 0;

				[[nodiscard]] static std::size_t _unitCount(std::size_t size) noexcept;

			public:
				void resize(std::size_t size);
				void reserve(std::size_t size);
				void append(const void *source, std::size_t size);
				void clear();
				[[nodiscard]] std::byte *data() noexcept;
				[[nodiscard]] const std::byte *data() const noexcept;
				[[nodiscard]] std::size_t size() const noexcept;
			};

			Storage _storage;
			Usage _usage = Usage::StaticDraw;

			[[nodiscard]] Kind _kind() const noexcept override;
			void _allocate(GPUResource::Instance &instance) const;

		protected:
			[[nodiscard]] std::unique_ptr<GPUResource::State> _clone() const override = 0;
			[[nodiscard]] virtual GLenum _target() const noexcept = 0;
			[[nodiscard]] std::unique_ptr<GPUResource::Instance> _create(RenderContext &context) const override;
			void _synchronize(GPUResource::Instance &instance, RenderContext &context) const override;
			void _bind(GPUResource::Instance &instance, RenderContext &context) const override;

		public:
			[[nodiscard]] Usage usage() const noexcept
			{
				return _usage;
			}
			[[nodiscard]] std::size_t size() const noexcept
			{
				return _storage.size();
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

		void _append(const void *data, std::size_t size);
		void _write(const void *data, std::size_t size, std::size_t offset = 0);
		void _resize(std::size_t size);
		void _reserve(std::size_t size);
		[[nodiscard]] std::byte *_data() noexcept;
		[[nodiscard]] const std::byte *_data() const noexcept;

	public:
		void clear();
		void setUsage(Usage usage);
		[[nodiscard]] Usage usage() const noexcept;
		[[nodiscard]] std::size_t size() const noexcept;
		std::vector<std::byte> retrieve(RenderContext &context) const;
	};
}
