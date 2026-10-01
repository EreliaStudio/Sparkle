#pragma once

#include "container/pool.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace spk
{
	class Message
	{
	public:
		using Type = std::uint32_t;
		using RequestID = std::uint64_t;

		struct Storage final : public std::vector<std::byte>
		{
			using Base = std::vector<std::byte>;
			using Pool = spk::Pool<Storage>;
			using Lease = Pool::Lease;

			using Base::Base;
		};

		class Reader
		{
		private:
			std::shared_ptr<const Storage::Lease> _storage;
			mutable std::size_t _readOffset = 0;

			explicit Reader(
				std::shared_ptr<const Storage::Lease> storage,
				std::size_t offset = 0);

			[[nodiscard]] std::size_t _size() const noexcept;
			void _seek(std::size_t offset) const;

			friend class Message;

		public:
			void reset() const noexcept;
			void seek(std::size_t offset) const;
			void skip(std::size_t size) const;
			void pull(void *data, std::size_t size) const;
			void readAt(std::size_t offset, void *destination, std::size_t size) const;

			template <typename TValue>
				requires std::is_trivially_copyable_v<TValue>
			void skip() const
			{
				skip(sizeof(TValue));
			}

			[[nodiscard]] std::size_t readOffset() const noexcept;
			[[nodiscard]] std::span<const std::byte> data() const noexcept;
			[[nodiscard]] std::size_t size() const noexcept;
			[[nodiscard]] bool empty() const noexcept;

			template <typename TValue>
				requires std::is_trivially_copyable_v<TValue>
			const Reader &operator>>(TValue &value) const
			{
				pull(&value, sizeof(TValue));
				return *this;
			}

			const Reader &operator>>(std::string &value) const;

			template <typename TValue>
				requires std::is_trivially_copyable_v<TValue>
			[[nodiscard]] TValue get() const
			{
				std::array<std::byte, sizeof(TValue)> bytes;
				pull(bytes.data(), bytes.size());
				return std::bit_cast<TValue>(bytes);
			}

			template <typename TValue>
				requires std::is_trivially_copyable_v<TValue>
			[[nodiscard]] TValue peek() const
			{
				return readAt<TValue>(_readOffset);
			}

			template <typename TValue>
				requires std::is_trivially_copyable_v<TValue>
			[[nodiscard]] TValue readAt(std::size_t offset) const
			{
				std::array<std::byte, sizeof(TValue)> bytes;
				readAt(offset, bytes.data(), bytes.size());
				return std::bit_cast<TValue>(bytes);
			}
		};

		class Writer
		{
		private:
			Type _type = 0;
			RequestID _requestID = 0;
			Storage::Lease _storage;

			void _ensureCapacity(std::size_t requiredCapacity);

		public:
			explicit Writer(Type type = 0) noexcept;
			explicit Writer(Message &&message);

			Writer(const Writer &) = delete;
			Writer(Writer &&) noexcept = default;
			~Writer() = default;

			Writer &operator=(const Writer &) = delete;
			Writer &operator=(Writer &&) noexcept = default;

			void setType(Type type) noexcept;
			[[nodiscard]] Type type() const noexcept;
			void setRequestID(RequestID requestID) noexcept;
			[[nodiscard]] RequestID requestID() const noexcept;

			void clear() noexcept;
			void resize(std::size_t size);
			void edit(std::size_t offset, const void *data, std::size_t size);
			void append(const void *data, std::size_t size);
			void push(const void *data, std::size_t size);

			template <typename TValue>
				requires std::is_trivially_copyable_v<TValue>
			void edit(std::size_t offset, const TValue &value)
			{
				edit(offset, &value, sizeof(TValue));
			}

			template <typename TValue>
				requires std::is_trivially_copyable_v<TValue>
			void append(const TValue &value)
			{
				append(&value, sizeof(TValue));
			}

			template <typename TValue>
				requires std::is_trivially_copyable_v<TValue>
			void push(const TValue &value)
			{
				append(value);
			}

			[[nodiscard]] std::span<std::byte> data() noexcept;
			[[nodiscard]] std::span<const std::byte> data() const noexcept;
			[[nodiscard]] std::size_t size() const noexcept;
			[[nodiscard]] std::size_t capacity() const noexcept;
			[[nodiscard]] bool empty() const noexcept;

			template <typename TValue>
				requires std::is_trivially_copyable_v<TValue>
			Writer &operator<<(const TValue &value)
			{
				append(value);
				return *this;
			}

			Writer &operator<<(std::string_view value);
			Writer &operator<<(const std::string &value)
			{
				return *this << std::string_view(value);
			}

			[[nodiscard]] Message build() &&;
		};

	private:
		Type _type = 0;
		RequestID _requestID = 0;
		std::shared_ptr<Storage::Lease> _storage;

		explicit Message(
			Type type,
			RequestID requestID,
			Storage::Lease storage);

		[[nodiscard]] static Storage::Lease _obtainStorage(std::size_t minimumCapacity);

	public:
		Message(const Message &) = default;
		Message(Message &&) noexcept = default;
		~Message() = default;

		Message &operator=(const Message &) = default;
		Message &operator=(Message &&) noexcept = default;

		[[nodiscard]] Type type() const noexcept;
		[[nodiscard]] RequestID requestID() const noexcept;
		[[nodiscard]] std::span<const std::byte> data() const noexcept;
		[[nodiscard]] std::size_t size() const noexcept;
		[[nodiscard]] bool empty() const noexcept;

		[[nodiscard]] Reader reader(std::size_t offset = 0) const;
	};
}
