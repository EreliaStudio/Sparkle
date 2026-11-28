#pragma once

#include <GL/glew.h>

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "graphics/opengl/buffer_gpu_resource.hpp"

namespace spk
{
	class IndexBuffer final : public BufferGPUResource
	{
	public:
		enum class Type
		{
			UnsignedByte,
			UnsignedShort,
			UnsignedInt
		};

	public:
		class State final : public BufferGPUResource::State
		{
			friend class IndexBuffer;

		private:
			std::optional<Type> _type;
			std::size_t _stride = 0;

		protected:
			[[nodiscard]] std::unique_ptr<GPUResource::State> _clone() const override
			{
				return std::make_unique<State>(*this);
			}
			[[nodiscard]] GLenum _target() const noexcept override;
			void _validateBuffer(const Buffer &buffer) const override;

		public:
			[[nodiscard]] std::optional<Type> type() const noexcept
			{
				return _type;
			}
			[[nodiscard]] std::size_t stride() const noexcept
			{
				return _stride;
			}
		};
		using Handle = GPUResource::Handle<State>;

	private:
		[[nodiscard]] static std::size_t _typeSize(Type type) noexcept;

		template <typename TIndex>
		void _validateType() const
		{
			static_assert(std::is_integral_v<TIndex>, "IndexBuffer requires an integral type.");
			static_assert(std::is_unsigned_v<TIndex>, "IndexBuffer requires an unsigned type.");
			const auto &content = state<State>();
			if (!content._type.has_value())
			{
				throw std::logic_error("IndexBuffer has no configured type");
			}
			if (sizeof(TIndex) != content._stride)
			{
				throw std::logic_error("Index type size does not match the configured IndexBuffer stride");
			}
		}

	public:
		IndexBuffer();
		[[nodiscard]] std::unique_ptr<GPUResource> clone() const override
		{
			auto result = std::make_unique<IndexBuffer>(*this);
			result->_setState(_cloneState());
			return result;
		}
		[[nodiscard]] Handle handle() const
		{
			return createHandle<State>();
		}

		void setType(Type type);
		void clearConfiguration();

		template <typename TIndex>
		[[nodiscard]] std::span<const TIndex> cast() const
		{
			_validateType<TIndex>();
			return buffer() != nullptr ? buffer()->cast<TIndex>() : std::span<const TIndex>{};
		}

		[[nodiscard]] bool isConfigured() const noexcept;
		[[nodiscard]] std::optional<Type> type() const noexcept;
		[[nodiscard]] std::size_t stride() const noexcept;
		[[nodiscard]] std::size_t count() const noexcept;
	};
}
