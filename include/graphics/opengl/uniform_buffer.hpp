#pragma once

#include <GL/glew.h>

#include <cstddef>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "graphics/opengl/buffer_gpu_resource.hpp"

namespace spk
{
	class RenderContext;

	class UniformBuffer : public BufferGPUResource
	{
	public:
		class State final : public BufferGPUResource::State
		{
			friend class UniformBuffer;

		private:
			std::size_t _bindingPoint = 0;
			std::optional<std::size_t> _expectedSize;

		protected:
			[[nodiscard]] std::unique_ptr<GPUResource::State> _clone() const override
			{
				return std::make_unique<State>(*this);
			}
			[[nodiscard]] GLenum _target() const noexcept override;
			void _validateBuffer(const Buffer &buffer) const override;
			void _bind(GPUResource::Instance &instance, RenderContext &context) const override;

		public:
			explicit State(std::size_t bindingPoint, std::optional<std::size_t> expectedSize = {}) :
				_bindingPoint(bindingPoint),
				_expectedSize(expectedSize)
			{
			}
		};
		using Handle = GPUResource::Handle<State>;

	public:
		UniformBuffer();
		[[nodiscard]] std::unique_ptr<GPUResource> clone() const override
		{
			auto result = std::make_unique<UniformBuffer>(*this);
			result->_setState(_cloneState());
			return result;
		}
		explicit UniformBuffer(std::size_t bindingPoint, std::size_t size);
		[[nodiscard]] Handle handle() const
		{
			return createHandle<State>();
		}

		[[nodiscard]] std::size_t bindingPoint() const noexcept;

		template <typename TType>
		[[nodiscard]] const TType &cast() const
		{
			static_assert(std::is_trivially_copyable_v<TType>);
			if (buffer() == nullptr || size() != sizeof(TType))
			{
				throw std::logic_error("UniformBuffer size is different than the requested type");
			}
			return buffer()->cast<TType>().front();
		}

		template <typename TType>
		[[nodiscard]] TType retrieve(RenderContext &context) const
		{
			static_assert(std::is_trivially_copyable_v<TType>, "UniformBuffer requires a trivially copyable type.");

			const auto data = BufferGPUResource::retrieve(context);

			if (data.size() != sizeof(TType))
			{
				throw std::logic_error("UniformBuffer size is different than the requested type");
			}

			TType result{};
			std::memcpy(std::addressof(result), data.data(), sizeof(TType));

			return result;
		}
	};
}
