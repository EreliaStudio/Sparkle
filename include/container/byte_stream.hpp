#pragma once

#include "exception.hpp"

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace spk
{
	template <typename TValue>
	concept ByteStreamRawValue =
		std::is_trivially_copyable_v<TValue> &&
		!std::is_pointer_v<TValue> &&
		!std::is_member_pointer_v<TValue> &&
		!std::is_array_v<TValue> &&
		!std::same_as<std::remove_cv_t<TValue>, std::string_view>;

	template <typename TValue>
	concept ByteStreamVectorElement = std::same_as<TValue, std::string> || (ByteStreamRawValue<TValue> && !std::same_as<TValue, bool>);

	template <typename TValue, typename TWriter, typename TReader>
	concept ByteStreamSerializable =
		!std::is_trivially_copyable_v<TValue> &&
		!std::same_as<TValue, std::string> &&
		std::default_initializable<TValue> &&
		requires(TWriter &writer, const TValue &source, const TReader &reader, TValue &result) {
			writer << source;
			reader >> result;
		};

	class ByteStream
	{
	public:
		using Buffer = std::vector<std::byte>;

		class Slice
		{
		private:
			std::shared_ptr<const Buffer> _buffer;
			std::size_t _begin = 0;
			std::size_t _end = 0;
			mutable std::size_t _cursor = 0;

			friend class ByteStream;
			Slice(std::shared_ptr<const Buffer> buffer, std::size_t begin, std::size_t end) :
				_buffer(std::move(buffer)),
				_begin(begin),
				_end(end)
			{
			}

		public:
			[[nodiscard]] std::size_t size() const noexcept
			{
				return _end - _begin;
			}
			[[nodiscard]] std::size_t readOffset() const noexcept
			{
				return _cursor;
			}
			[[nodiscard]] std::size_t remaining() const noexcept
			{
				return size() - _cursor;
			}
			[[nodiscard]] bool empty() const noexcept
			{
				return size() == 0;
			}
			void reset() const noexcept
			{
				_cursor = 0;
			}

			void seek(std::size_t offset) const
			{
				if (offset > size())
				{
					throw spk::Exception("ByteStream slice seek outside bounds.");
				}
				_cursor = offset;
			}

			void skip(std::size_t count) const
			{
				if (count > remaining())
				{
					throw spk::Exception("ByteStream slice skip outside bounds.");
				}
				_cursor += count;
			}

			[[nodiscard]] std::span<const std::byte> data() const noexcept
			{
				if (size() == 0)
				{
					return {};
				}
				return std::span<const std::byte>(_buffer->data() + _begin, size());
			}

			void pull(void *destination, std::size_t count) const
			{
				if (count > remaining())
				{
					throw spk::Exception("ByteStream slice read outside bounds.");
				}
				if (count != 0)
				{
					std::memcpy(destination, _buffer->data() + _begin + _cursor, count);
				}
				_cursor += count;
			}

			template <typename TValue>
				requires std::is_trivially_copyable_v<TValue>
			const Slice &operator>>(TValue &value) const
			{
				pull(&value, sizeof(TValue));
				return *this;
			}

			const Slice &operator>>(std::string &value) const
			{
				std::uint32_t length = 0;
				*this >> length;
				if (length > remaining())
				{
					throw spk::Exception("ByteStream string exceeds slice bounds.");
				}
				std::string decoded(length, '\0');
				pull(decoded.data(), length);
				value = std::move(decoded);
				return *this;
			}

			template <typename TValue>
				requires ByteStreamVectorElement<TValue>
			const Slice &operator>>(std::vector<TValue> &value) const
			{
				std::uint32_t count = 0;
				*this >> count;
				const std::size_t minimum = std::same_as<TValue, std::string> ? sizeof(std::uint32_t) : sizeof(TValue);
				if (count > remaining() / minimum)
				{
					throw spk::Exception("ByteStream vector exceeds slice bounds.");
				}
				std::vector<TValue> decoded;
				if constexpr (ByteStreamRawValue<TValue>)
				{
					decoded.resize(count);
					pull(decoded.data(), decoded.size() * sizeof(TValue));
				}
				else
				{
					decoded.reserve(count);
					for (std::uint32_t i = 0; i < count; ++i)
					{
						std::string element;
						*this >> element;
						decoded.push_back(std::move(element));
					}
				}
				value = std::move(decoded);
				return *this;
			}

			[[nodiscard]] Slice slice(std::size_t begin, std::size_t end) const
			{
				if (begin > end || end > size())
				{
					throw spk::Exception("Invalid nested ByteStream slice bounds.");
				}
				return Slice(_buffer, _begin + begin, _begin + end);
			}
		};

		class Writer
		{
		private:
			Buffer _buffer;

		public:
			void clear() noexcept
			{
				_buffer.clear();
			}
			[[nodiscard]] std::size_t size() const noexcept
			{
				return _buffer.size();
			}
			[[nodiscard]] std::span<const std::byte> data() const noexcept
			{
				return _buffer;
			}

			void append(const void *source, std::size_t count)
			{
				if (count > _buffer.max_size() - _buffer.size())
				{
					throw spk::Exception("ByteStream buffer size overflow.");
				}
				if (count == 0)
				{
					return;
				}
				const auto *bytes = static_cast<const std::byte *>(source);
				_buffer.insert(_buffer.end(), bytes, bytes + count);
			}

			template <typename TValue>
				requires std::is_trivially_copyable_v<TValue>
			Writer &operator<<(const TValue &value)
			{
				append(&value, sizeof(TValue));
				return *this;
			}

			Writer &operator<<(std::string_view value)
			{
				if (value.size() > std::numeric_limits<std::uint32_t>::max())
				{
					throw spk::Exception("ByteStream string exceeds maximum length.");
				}
				*this << static_cast<std::uint32_t>(value.size());
				append(value.data(), value.size());
				return *this;
			}

			Writer &operator<<(const std::string &value)
			{
				return *this << std::string_view(value);
			}

			template <typename TValue>
				requires ByteStreamVectorElement<TValue>
			Writer &operator<<(const std::vector<TValue> &value)
			{
				if (value.size() > std::numeric_limits<std::uint32_t>::max())
				{
					throw spk::Exception("ByteStream vector exceeds maximum element count.");
				}
				*this << static_cast<std::uint32_t>(value.size());
				if constexpr (ByteStreamRawValue<TValue>)
				{
					if (value.size() > _buffer.max_size() / sizeof(TValue))
					{
						throw spk::Exception("ByteStream vector exceeds maximum byte count.");
					}
					append(value.data(), value.size() * sizeof(TValue));
				}
				else
				{
					for (const auto &element : value)
					{
						*this << element;
					}
				}
				return *this;
			}

			[[nodiscard]] ByteStream build() &&
			{
				return ByteStream(std::make_shared<const Buffer>(std::move(_buffer)));
			}
		};

	private:
		std::shared_ptr<const Buffer> _buffer;
		explicit ByteStream(std::shared_ptr<const Buffer> buffer) :
			_buffer(std::move(buffer))
		{
		}

	public:
		ByteStream() :
			_buffer(std::make_shared<const Buffer>())
		{
		}

		template <typename TValue>
			requires ByteStreamRawValue<TValue>
		explicit ByteStream(const TValue &value)
		{
			Buffer bytes(sizeof(TValue));
			std::memcpy(bytes.data(), &value, sizeof(TValue));
			_buffer = std::make_shared<const Buffer>(std::move(bytes));
		}

		explicit ByteStream(std::string_view value)
		{
			Writer writer;
			writer << value;
			*this = std::move(writer).build();
		}

		explicit ByteStream(const std::string &value) :
			ByteStream(std::string_view(value))
		{
		}

		template <typename TValue>
			requires ByteStreamSerializable<TValue, Writer, Slice>
		explicit ByteStream(const TValue &value)
		{
			Writer payload;
			payload << value;
			if (payload.size() > std::numeric_limits<std::uint32_t>::max())
			{
				throw spk::Exception("ByteStream object exceeds maximum serialized size.");
			}
			Writer writer;
			writer << static_cast<std::uint32_t>(payload.size());
			writer.append(payload.data().data(), payload.size());
			*this = std::move(writer).build();
		}
		[[nodiscard]] static ByteStream share(std::shared_ptr<const Buffer> buffer)
		{
			if (!buffer)
			{
				throw spk::Exception("ByteStream requires valid storage.");
			}
			return ByteStream(std::move(buffer));
		}
		[[nodiscard]] std::size_t size() const noexcept
		{
			return _buffer->size();
		}
		[[nodiscard]] bool empty() const noexcept
		{
			return _buffer->empty();
		}
		[[nodiscard]] std::span<const std::byte> data() const noexcept
		{
			return *_buffer;
		}

		template <typename TValue>
			requires ByteStreamRawValue<TValue>
		[[nodiscard]] TValue cast() const
		{
			if (size() < sizeof(TValue))
			{
				throw spk::Exception("ByteStream contains too few bytes for the requested type.");
			}
			std::array<std::byte, sizeof(TValue)> bytes;
			std::memcpy(bytes.data(), data().data(), bytes.size());
			return std::bit_cast<TValue>(bytes);
		}

		template <typename TValue>
			requires std::same_as<TValue, std::string>
		[[nodiscard]] TValue cast() const
		{
			auto reader = this->reader();
			TValue value;
			reader >> value;
			return value;
		}

		template <typename TValue>
			requires ByteStreamSerializable<TValue, Writer, Slice>
		[[nodiscard]] TValue cast() const
		{
			auto reader = this->reader();
			std::uint32_t length = 0;
			reader >> length;
			if (length > reader.remaining())
			{
				throw spk::Exception("ByteStream object exceeds stream bounds.");
			}
			auto payload = reader.slice(reader.readOffset(), reader.readOffset() + length);
			TValue value{};
			payload >> value;
			return value;
		}

		[[nodiscard]] Slice slice(std::size_t begin, std::size_t end) const
		{
			if (begin > end || end > size())
			{
				throw spk::Exception("Invalid ByteStream slice bounds.");
			}
			return Slice(_buffer, begin, end);
		}

		[[nodiscard]] Slice reader() const
		{
			return slice(0, size());
		}
	};
}
