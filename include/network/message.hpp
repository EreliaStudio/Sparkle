#pragma once

#include "container/byte_stream.hpp"

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

		struct Header
		{
			Type messageType = 0;
			RequestID requestID = 0;
		};

		using Reader = spk::ByteStream::Reader;

		class Writer
		{
		private:
			Header _header{};
			spk::ByteStream::Writer _payload;

		public:
			explicit Writer(Type type = 0) noexcept;
			explicit Writer(Message &&message);
			Writer(const Writer &) = delete;
			Writer(Writer &&) noexcept = default;
			Writer &operator=(const Writer &) = delete;
			Writer &operator=(Writer &&) noexcept = default;

			[[nodiscard]] Header &header() noexcept;
			[[nodiscard]] const Header &header() const noexcept;
			[[nodiscard]] spk::ByteStream::Writer &payload() noexcept;
			[[nodiscard]] const spk::ByteStream::Writer &payload() const noexcept;
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
				payload() << value;
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
		Header _header;
		spk::ByteStream _payload;

		explicit Message(Header header, spk::ByteStream payload);

	public:
		Message(const Message &) = default;
		Message(Message &&) noexcept = default;
		~Message() = default;
		Message &operator=(const Message &) = default;
		Message &operator=(Message &&) noexcept = default;

		[[nodiscard]] const Header &header() const noexcept;
		[[nodiscard]] const spk::ByteStream &payload() const noexcept;
		[[nodiscard]] spk::ByteStream &payload() noexcept;
		[[nodiscard]] Type type() const noexcept;
		[[nodiscard]] RequestID requestID() const noexcept;
		[[nodiscard]] std::span<const std::byte> data() const noexcept;
		[[nodiscard]] std::size_t size() const noexcept;
		[[nodiscard]] bool empty() const noexcept;
		[[nodiscard]] Reader reader(std::size_t offset = 0) const;
	};

	template <typename T>
	concept MessageSerializable = requires(Message::Writer &writer, const Message::Reader &reader, const T &input, T &output) {
		writer << input;
		reader >> output;
	};
}
