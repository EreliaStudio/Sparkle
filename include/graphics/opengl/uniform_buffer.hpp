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

	class UniformBuffer final : public BufferGPUResource
	{
	private:
		std::size_t _bindingPoint = 0;

		using BufferGPUResource::clear;

	protected:
		[[nodiscard]] GLenum _target() const noexcept override;
		void _bind(GPUResource::Instance &instance, RenderContext &context) const override;

	public:
		UniformBuffer() = default;
		explicit UniformBuffer(std::size_t bindingPoint, std::size_t size);

		[[nodiscard]] std::size_t bindingPoint() const noexcept;

		void setData(const void *data, std::size_t size);
		void setData(const std::vector<std::byte> &data);

		template <typename TType>
		void setData(const TType &data)
		{
			static_assert(std::is_trivially_copyable_v<TType>, "UniformBuffer requires a trivially copyable type.");

			if (sizeof(TType) != size())
			{
				throw std::logic_error("UniformBuffer size is different than the provided type");
			}

			setData(std::addressof(data), sizeof(TType));
		}

		template <typename TType>
		[[nodiscard]] TType &cast()
		{
			static_assert(std::is_trivially_copyable_v<TType>, "UniformBuffer requires a trivially copyable type.");
			if (size() != sizeof(TType))
			{
				throw std::logic_error("UniformBuffer size is different than the requested type");
			}

			return *reinterpret_cast<TType *>(_data());
		}

		template <typename TType>
		[[nodiscard]] const TType &cast() const
		{
			static_assert(std::is_trivially_copyable_v<TType>, "UniformBuffer requires a trivially copyable type.");
			if (size() != sizeof(TType))
			{
				throw std::logic_error("UniformBuffer size is different than the requested type");
			}

			return *reinterpret_cast<const TType *>(_data());
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