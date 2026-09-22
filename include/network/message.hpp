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
		using Storage = std::vector<std::byte>;

	private:
		Type _type = 0;
		Storage _payload;
		mutable std::size_t _readOffset = 0;

	public:
		Message() = default;
		explicit Message(Type type) noexcept;
		Message(Type type, Storage payload) noexcept;

		void setType(Type type) noexcept;
		[[nodiscard]] Type type() const noexcept;

		void clear() noexcept;
		void reset() const noexcept;
		void resize(std::size_t size);
		void skip(std::size_t size) const;
		void edit(std::size_t offset, const void *data, std::size_t size);
		void append(const void *data, std::size_t size);
		void push(const void *data, std::size_t size);
		void pull(void *data, std::size_t size) const;

		template <typename TValue>
			requires std::is_trivially_copyable_v<TValue>
		void skip() const
		{
			skip(sizeof(TValue));
		}

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

		template <typename TValue>
			requires std::is_trivially_copyable_v<TValue>
		void pull(TValue &value) const
		{
			pull(&value, sizeof(TValue));
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
			pull(value);
			return *this;
		}

		const Message &operator>>(std::string &value) const;

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
			const std::size_t offset = _readOffset;
			const TValue result = get<TValue>();
			_readOffset = offset;
			return result;
		}
	};
}
