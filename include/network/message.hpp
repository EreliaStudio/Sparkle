#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
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
		using Storage = std::vector<std::byte>;

		class Reader
		{
		private:
			const Message *_message = nullptr;
			mutable std::size_t _readOffset = 0;

			void _seek(std::size_t offset) const;

		public:
			explicit Reader(
				const Message &message,
				std::size_t offset = 0);

			void reset() const noexcept;
			void seek(std::size_t offset) const;
			void skip(std::size_t size) const;
			void pull(void *data, std::size_t size) const;

			template <typename TValue>
				requires std::is_trivially_copyable_v<TValue>
			void skip() const
			{
				skip(sizeof(TValue));
			}

			[[nodiscard]] std::size_t readOffset() const noexcept;

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
				return _message->readAt<TValue>(_readOffset);
			}
		};

	private:
		Type _type = 0;
		RequestID _requestID = 0;
		Storage _payload;
		mutable Reader _reader;

	public:
		Message() noexcept;
		explicit Message(Type type) noexcept;
		Message(Type type, Storage payload) noexcept;
		Message(const Message &other);
		Message(Message &&other) noexcept;
		Message &operator=(const Message &other);
		Message &operator=(Message &&other) noexcept;

		void setType(Type type) noexcept;
		[[nodiscard]] Type type() const noexcept;
		void setRequestID(RequestID requestID) noexcept;
		[[nodiscard]] RequestID requestID() const noexcept;

		void clear() noexcept;
		void reset() const noexcept;
		void resize(std::size_t size);
		void skip(std::size_t size) const;
		void edit(std::size_t offset, const void *data, std::size_t size);
		void readAt(std::size_t offset, void *destination, std::size_t size) const;
		void append(const void *data, std::size_t size);
		void push(const void *data, std::size_t size);
		void pull(void *data, std::size_t size) const;

		template <typename TValue>
			requires std::is_trivially_copyable_v<TValue>
		void skip() const
		{
			_reader.skip<TValue>();
		}

		template <typename TValue>
			requires std::is_trivially_copyable_v<TValue>
		void edit(std::size_t offset, const TValue &value)
		{
			edit(offset, &value, sizeof(TValue));
		}

		template <typename TValue>
			requires std::is_trivially_copyable_v<TValue>
		[[nodiscard]] TValue readAt(std::size_t offset) const
		{
			std::array<std::byte, sizeof(TValue)> bytes;
			readAt(offset, bytes.data(), bytes.size());
			return std::bit_cast<TValue>(bytes);
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

		[[nodiscard]] std::span<const std::byte> data() const noexcept;
		[[nodiscard]] std::size_t size() const noexcept;
		[[nodiscard]] bool empty() const noexcept;
		[[nodiscard]] std::size_t readOffset() const noexcept;

		template <typename TValue>
			requires std::is_trivially_copyable_v<TValue>
		Message &operator<<(const TValue &value)
		{
			append(value);
			return *this;
		}

		Message &operator<<(std::string_view value);
		Message &operator<<(const std::string &value)
		{
			return *this << std::string_view(value);
		}

		template <typename TValue>
			requires std::is_trivially_copyable_v<TValue>
		const Message &operator>>(TValue &value) const
		{
			_reader >> value;
			return *this;
		}

		const Message &operator>>(std::string &value) const;

		template <typename TValue>
			requires std::is_trivially_copyable_v<TValue>
		[[nodiscard]] TValue get() const
		{
			return _reader.get<TValue>();
		}

		template <typename TValue>
			requires std::is_trivially_copyable_v<TValue>
		[[nodiscard]] TValue peek() const
		{
			return _reader.peek<TValue>();
		}

		[[nodiscard]] Reader reader(std::size_t offset = 0) const;
	};
}
