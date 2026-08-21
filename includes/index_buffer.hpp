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

#include "buffer_gpu_resource.hpp"

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

	private:
		std::optional<Type> _type;
		std::size_t _stride = 0;

		[[nodiscard]] static std::size_t _typeSize(Type type) noexcept;

		template <typename TIndex>
		void _validateType() const
		{
			static_assert(std::is_integral_v<TIndex>, "IndexBuffer requires an integral type.");
			static_assert(std::is_unsigned_v<TIndex>, "IndexBuffer requires an unsigned type.");
			if (!_type.has_value())
			{
				throw std::logic_error("IndexBuffer has no configured type");
			}
			if (sizeof(TIndex) != _stride)
			{
				throw std::logic_error("Index type size does not match the configured IndexBuffer stride");
			}
		}

		[[nodiscard]] static std::size_t _checkedSize(std::size_t count, std::size_t stride);

	protected:
		[[nodiscard]] GLenum _target() const noexcept override;

	public:
		IndexBuffer() = default;

		void setType(Type type);
		void clearConfiguration();

		template <typename TIndex>
		void resize(std::size_t count)
		{
			_validateType<TIndex>();
			_resize(_checkedSize(count, _stride));
		}

		template <typename TIndex>
		void reserve(std::size_t count)
		{
			_validateType<TIndex>();
			_reserve(_checkedSize(count, _stride));
		}

		template <typename TIndex>
		void pushBack(const TIndex &index)
		{
			_validateType<TIndex>();
			_append(std::addressof(index), sizeof(TIndex));
		}

		template <typename TIndex, typename... TArguments>
		void emplace(TArguments &&...arguments)
		{
			_validateType<TIndex>();
			TIndex index(std::forward<TArguments>(arguments)...);
			_append(std::addressof(index), sizeof(TIndex));
		}

		template <typename TIndex>
		void insert(std::span<const TIndex> indices)
		{
			_validateType<TIndex>();
			_append(indices.data(), indices.size_bytes());
		}

		template <typename TIndex>
		void insert(std::initializer_list<TIndex> indices)
		{
			insert<TIndex>(std::span<const TIndex>(indices.begin(), indices.size()));
		}

		template <typename TIndex>
		[[nodiscard]] std::span<TIndex> cast()
		{
			_validateType<TIndex>();
			return {reinterpret_cast<TIndex *>(_data()), count()};
		}

		template <typename TIndex>
		[[nodiscard]] std::span<const TIndex> cast() const
		{
			_validateType<TIndex>();
			return {reinterpret_cast<const TIndex *>(_data()), count()};
		}

		[[nodiscard]] bool isConfigured() const noexcept;
		[[nodiscard]] std::optional<Type> type() const noexcept;
		[[nodiscard]] std::size_t stride() const noexcept;
		[[nodiscard]] std::size_t count() const noexcept;
	};
}
