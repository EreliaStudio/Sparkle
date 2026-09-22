#pragma once

#include <bit>
#include <concepts>
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

		template <typename TValue>
		static constexpr bool _wireScalar =
			std::integral<TValue> || std::floating_point<TValue> || std::is_enum_v<TValue>;

		template <std::unsigned_integral TValue>
		void _appendUnsigned(TValue value)
		{
			for (std::size_t index = 0; index < sizeof(TValue); ++index)
			{
				_payload.push_back(static_cast<std::byte>(value & 0xFFu));
				value >>= 8u;
			}
		}

		template <std::unsigned_integral TValue>
		[[nodiscard]] TValue _readUnsigned() const
		{
			TValue result = 0;
			for (std::size_t index = 0; index < sizeof(TValue); ++index)
			{
				result |= static_cast<TValue>(std::to_integer<std::uint8_t>(_readByte())) << (index * 8u);
			}
			return result;
		}

		[[nodiscard]] std::byte _readByte() const;

		template <typename TValue>
			requires(_wireScalar<TValue>)
		void _appendScalar(TValue value)
		{
			if constexpr (std::is_enum_v<TValue>)
			{
				_appendScalar(static_cast<std::underlying_type_t<TValue>>(value));
			}
			else if constexpr (std::same_as<TValue, bool>)
			{
				_appendUnsigned<std::uint8_t>(value ? 1u : 0u);
			}
			else if constexpr (std::floating_point<TValue>)
			{
				using Bits = std::conditional_t<sizeof(TValue) == 4, std::uint32_t, std::uint64_t>;
				_appendUnsigned(std::bit_cast<Bits>(value));
			}
			else
			{
				using Bits = std::make_unsigned_t<TValue>;
				_appendUnsigned(std::bit_cast<Bits>(value));
			}
		}

		template <typename TValue>
			requires(_wireScalar<TValue>)
		[[nodiscard]] TValue _readScalar() const
		{
			if constexpr (std::is_enum_v<TValue>)
			{
				return static_cast<TValue>(_readScalar<std::underlying_type_t<TValue>>());
			}
			else if constexpr (std::same_as<TValue, bool>)
			{
				return _readUnsigned<std::uint8_t>() != 0;
			}
			else if constexpr (std::floating_point<TValue>)
			{
				using Bits = std::conditional_t<sizeof(TValue) == 4, std::uint32_t, std::uint64_t>;
				return std::bit_cast<TValue>(_readUnsigned<Bits>());
			}
			else
			{
				using Bits = std::make_unsigned_t<TValue>;
				return std::bit_cast<TValue>(_readUnsigned<Bits>());
			}
		}

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

		[[nodiscard]] std::span<const std::byte> data() const noexcept;
		[[nodiscard]] std::size_t size() const noexcept;
		[[nodiscard]] bool empty() const noexcept;
		[[nodiscard]] std::size_t readOffset() const noexcept;

		template <typename TValue>
			requires(_wireScalar<TValue>)
		Message &operator<<(TValue value)
		{
			_appendScalar(value);
			return *this;
		}

		Message &operator<<(std::string_view value);
		Message &operator<<(const std::string &value)
		{
			return *this << std::string_view(value);
		}

		template <typename TValue>
			requires(_wireScalar<TValue>)
		const Message &operator>>(TValue &value) const
		{
			value = _readScalar<TValue>();
			return *this;
		}

		const Message &operator>>(std::string &value) const;

		template <typename TValue>
			requires(_wireScalar<TValue>)
		[[nodiscard]] TValue get() const
		{
			return _readScalar<TValue>();
		}

		template <typename TValue>
			requires(_wireScalar<TValue>)
		[[nodiscard]] TValue peek() const
		{
			const std::size_t offset = _readOffset;
			try
			{
				const TValue result = _readScalar<TValue>();
				_readOffset = offset;
				return result;
			}
			catch (...)
			{
				_readOffset = offset;
				throw;
			}
		}
	};
}
