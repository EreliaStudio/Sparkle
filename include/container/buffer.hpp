#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <type_traits>
#include <vector>

#include "container/capacity_pool.hpp"

namespace spk
{
	class Buffer
	{
	public:
		class Builder;
		class Immutable;
		class Mutable;

	private:
		class Data
		{
		private:
			std::vector<std::byte> _storage;
			std::uint64_t _version = 0;

		public:
			[[nodiscard]] std::byte *data();
			[[nodiscard]] const std::byte *data() const;
			[[nodiscard]] std::size_t size() const;
			[[nodiscard]] std::size_t capacity() const;
			[[nodiscard]] std::uint64_t version() const;
			void reserve(std::size_t capacity);
			void resize(std::size_t size);
			void clear();
			void reset();
			void append(std::span<const std::byte> bytes);
			void validate();
		};

		template <typename T>
		static constexpr void _checkType()
		{
			static_assert(std::is_trivially_copyable_v<T>);
			static_assert(!std::is_volatile_v<T>);
			static_assert(alignof(T) <= alignof(std::max_align_t));
#if defined(__cpp_lib_is_implicit_lifetime)
			static_assert(std::is_implicit_lifetime_v<T>);
#endif
		}

		template <typename T>
		[[nodiscard]] static std::size_t _byteCount(std::size_t count)
		{
			_checkType<T>();
			if (count > std::numeric_limits<std::size_t>::max() / sizeof(T))
			{
				throw spk::Exception("Buffer byte count overflow");
			}
			return count * sizeof(T);
		}

		template <typename T, typename TByte>
		[[nodiscard]] static std::span<T> _cast(TByte *bytes, std::size_t size)
		{
			_checkType<T>();
			if (size % sizeof(T) != 0 || reinterpret_cast<std::uintptr_t>(bytes) % alignof(T) != 0)
			{
				throw spk::Exception("Buffer storage is incompatible with the requested type");
			}
			return {reinterpret_cast<T *>(bytes), size / sizeof(T)};
		}

	protected:
		[[nodiscard]] virtual const Data &_getData() const = 0;

	public:
		virtual ~Buffer() = default;

		[[nodiscard]] const std::byte *data() const;
		[[nodiscard]] std::size_t size() const;
		[[nodiscard]] std::uint64_t version() const;
		[[nodiscard]] bool empty() const;

		template <typename T>
		[[nodiscard]] std::span<const T> cast() const
		{
			return _cast<const T>(data(), size());
		}
	};

	class Buffer::Immutable final : public Buffer
	{
		friend class Builder;

	private:
		std::shared_ptr<const Data> _data;
		explicit Immutable(std::shared_ptr<const Data> data);

	protected:
		[[nodiscard]] const Data &_getData() const override;

	public:
		Immutable(const Immutable &) = default;
		Immutable(Immutable &&) noexcept = default;
		Immutable &operator=(const Immutable &) = default;
		Immutable &operator=(Immutable &&) noexcept = default;
	};

	class Buffer::Mutable final : public Buffer
	{
		friend class Builder;

	private:
		std::shared_ptr<Data> _data;
		explicit Mutable(std::shared_ptr<Data> data);
		[[nodiscard]] Data &_editableData();

	protected:
		[[nodiscard]] const Data &_getData() const override;

	public:
		Mutable(const Mutable &) = default;
		Mutable(Mutable &&) noexcept = default;
		Mutable &operator=(const Mutable &) = default;
		Mutable &operator=(Mutable &&) noexcept = default;

		using Buffer::data;
		using Buffer::cast;

		[[nodiscard]] std::byte *data();

		template <typename T>
		[[nodiscard]] std::span<T> cast()
		{
			return _cast<T>(data(), size());
		}

		void reserve(std::size_t capacity);
		void resize(std::size_t size);
		void clear();
		void validate();

		template <typename T>
		void reserve(std::size_t count)
		{
			reserve(_byteCount<T>(count));
		}

		template <typename T>
		void resize(std::size_t count)
		{
			resize(_byteCount<T>(count));
		}

		template <typename T, std::size_t TExtent>
		void insert(std::span<T, TExtent> values)
		{
			_checkType<T>();
			_editableData().append(std::as_bytes(values));
		}

		template <typename T>
		void pushBack(const T &value)
		{
			insert(std::span<const T>(&value, 1));
		}
	};

	class Buffer::Builder
	{
	private:
		static CapacityPool<Data> _pool;
		CapacityPool<Data>::Lease _data;
		bool _consumed = false;

		void _check() const;
		void _ensureData(std::size_t capacity);
		void _append(std::span<const std::byte> bytes);
		[[nodiscard]] std::shared_ptr<Data> _publish();

	public:
		Builder() = default;
		Builder(const Builder &) = delete;
		Builder &operator=(const Builder &) = delete;
		Builder(Builder &&other) noexcept;
		Builder &operator=(Builder &&other) noexcept;

		void reserve(std::size_t capacity);
		void resize(std::size_t size);
		void clear();

		[[nodiscard]] std::size_t size() const;
		[[nodiscard]] bool empty() const;
		[[nodiscard]] std::byte *data();
		[[nodiscard]] const std::byte *data() const;

		template <typename T>
		void reserve(std::size_t count)
		{
			reserve(_byteCount<T>(count));
		}

		template <typename T>
		void resize(std::size_t count)
		{
			resize(_byteCount<T>(count));
		}

		template <typename T, std::size_t TExtent>
		void insert(std::span<T, TExtent> values)
		{
			_checkType<T>();
			_append(std::as_bytes(values));
		}

		template <typename T>
		void pushBack(const T &value)
		{
			insert(std::span<const T>(&value, 1));
		}

		template <typename T>
		[[nodiscard]] std::span<T> cast()
		{
			return _cast<T>(data(), size());
		}

		template <typename T>
		[[nodiscard]] std::span<const T> cast() const
		{
			return _cast<const T>(data(), size());
		}

		[[nodiscard]] Immutable build() &&;
		[[nodiscard]] Mutable buildMutable() &&;
	};
}
