#pragma once

#include "container/pool.hpp"
#include "exception.hpp"

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace spk
{
	template <typename TValue, typename TWriter, typename TReader>
	concept ByteStreamSerializable =
		std::default_initializable<TValue> &&
		requires(TWriter &writer, const TReader &reader, const TValue &source, TValue &destination) {
			writer << source;
			reader >> destination;
		};

	// Dynamically sized collections with value insertion (vectors, sets, maps, etc.).
	template <typename TCollection>
	concept ByteStreamCollection =
		std::ranges::sized_range<const TCollection> &&
		!std::same_as<TCollection, std::string> &&
		requires(TCollection &collection, typename TCollection::value_type value) {
			{ collection.clear() } -> std::same_as<void>;
		} &&
		(requires(TCollection &collection, typename TCollection::value_type value) {
			collection.push_back(std::move(value));
		} || requires(TCollection &collection, typename TCollection::value_type value) {
			collection.insert(std::move(value));
		});

	template <typename TCollection>
	concept ByteStreamMap = ByteStreamCollection<TCollection> &&
		requires {
			typename TCollection::key_type;
			typename TCollection::mapped_type;
		};

	template <typename TCollection, typename TWriter, typename TReader>
	concept ByteStreamCollectionSerializable = ByteStreamCollection<TCollection> &&
		((ByteStreamMap<TCollection> &&
		  ByteStreamSerializable<typename TCollection::key_type, TWriter, TReader> &&
		  ByteStreamSerializable<typename TCollection::mapped_type, TWriter, TReader>) ||
		 (!ByteStreamMap<TCollection> &&
		  ByteStreamSerializable<typename TCollection::value_type, TWriter, TReader>));

	class ByteStream
	{
	public:
		using Buffer = std::vector<std::byte>;
		class Writer;

		class Reader
		{
		private:
			std::shared_ptr<const Buffer> _buffer;
			std::size_t _begin = 0;
			std::size_t _end = 0;
			mutable std::size_t _cursor = 0;

			friend class ByteStream;
			Reader(std::shared_ptr<const Buffer> buffer, std::size_t begin, std::size_t end) :
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
					throw spk::Exception("ByteStream reader seek outside bounds.");
				}
				_cursor = offset;
			}

			void skip(std::size_t count) const
			{
				if (count > remaining())
				{
					throw spk::Exception("ByteStream reader skip outside bounds.");
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
					throw spk::Exception("ByteStream reader read outside bounds.");
				}
				if (count != 0)
				{
					std::memcpy(destination, _buffer->data() + _begin + _cursor, count);
				}
				_cursor += count;
			}

			template <typename TValue>
				requires(std::is_trivially_copyable_v<TValue> && !std::is_pointer_v<TValue> && !std::is_member_pointer_v<TValue> && !std::is_array_v<TValue> && !std::same_as<TValue, std::string_view>)
			const Reader &operator>>(TValue &value) const
			{
				pull(&value, sizeof(TValue));
				return *this;
			}

			void readAt(std::size_t offset, void *destination, std::size_t count) const
			{
				if (offset > size() || count > size() - offset)
				{
					throw spk::Exception("ByteStream reader read outside bounds.");
				}
				subreader(offset, offset + count).pull(destination, count);
			}

			template <typename TValue>
				requires std::is_trivially_copyable_v<TValue>
			void skip() const
			{
				skip(sizeof(TValue));
			}

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
				return readAt<TValue>(readOffset());
			}

			template <typename TValue>
				requires std::is_trivially_copyable_v<TValue>
			[[nodiscard]] TValue readAt(std::size_t offset) const
			{
				std::array<std::byte, sizeof(TValue)> bytes;
				readAt(offset, bytes.data(), bytes.size());
				return std::bit_cast<TValue>(bytes);
			}

			const Reader &operator>>(std::string &value) const
			{
				std::uint32_t length = 0;
				*this >> length;
				if (length > remaining())
				{
					throw spk::Exception("ByteStream string exceeds reader bounds.");
				}
				std::string decoded(length, '\0');
				pull(decoded.data(), length);
				value = std::move(decoded);
				return *this;
			}

			template <typename TCollection>
				requires ByteStreamCollectionSerializable<TCollection, Writer, Reader>
			const Reader &operator>>(TCollection &value) const
			{
				std::uint32_t count = 0;
				*this >> count;
				using Element = typename TCollection::value_type;
				constexpr bool bulk = !ByteStreamMap<TCollection> &&
					std::ranges::contiguous_range<TCollection> &&
					std::is_trivially_copyable_v<Element> &&
					!std::same_as<Element, bool> &&
					requires(TCollection &collection, std::size_t n) {
						collection.resize(n);
					};
				if constexpr (bulk)
				{
					if (count > remaining() / sizeof(Element))
					{
						throw spk::Exception("ByteStream collection exceeds reader bounds.");
					}
					TCollection decoded;
					decoded.resize(count);
					pull(std::ranges::data(decoded), decoded.size() * sizeof(Element));
					value = std::move(decoded);
				}
				else
				{
					_readCollectionElements(value, count);
				}
				return *this;
			}

		private:
			template <typename TCollection>
			void _readCollectionElements(TCollection &value, std::uint32_t count) const
			{
				using Element = typename TCollection::value_type;
				constexpr std::size_t minimum = ByteStreamMap<TCollection> ? 2 :
					(std::same_as<Element, std::string> ? sizeof(std::uint32_t) : 1);
				if (count > remaining() / minimum)
				{
					throw spk::Exception("ByteStream collection exceeds reader bounds.");
				}
				TCollection decoded;
				for (std::uint32_t index = 0; index < count; ++index)
				{
					_readCollectionElement(decoded);
				}
				value = std::move(decoded);
			}

			template <typename TCollection>
			void _readCollectionElement(TCollection &decoded) const
			{
				if constexpr (ByteStreamMap<TCollection>)
				{
					typename TCollection::key_type key{};
					typename TCollection::mapped_type mapped{};
					*this >> key >> mapped;
					decoded.emplace(std::move(key), std::move(mapped));
				}
				else
				{
					typename TCollection::value_type element{};
					*this >> element;
					if constexpr (requires { decoded.push_back(std::move(element)); })
					{
						decoded.push_back(std::move(element));
					}
					else
					{
						decoded.insert(std::move(element));
					}
				}
			}

		public:

			[[nodiscard]] Reader subreader(std::size_t begin, std::size_t end) const
			{
				if (begin > end || end > size())
				{
					throw spk::Exception("Invalid nested ByteStream reader bounds.");
				}
				return Reader(_buffer, _begin + begin, _begin + end);
			}
		};

		class Writer
		{
		private:
			std::shared_ptr<Buffer> _buffer;

			[[nodiscard]] static std::shared_ptr<Buffer> _acquire()
			{
				static spk::Pool<Buffer> pool;
				auto lease = pool.obtain([](Buffer &buffer) {
					buffer.clear();
				});
				auto owner = std::make_shared<spk::Pool<Buffer>::Lease>(std::move(lease));
				return std::shared_ptr<Buffer>(owner, owner->get());
			}

			void _ensureBuffer()
			{
				if (!_buffer)
				{
					_buffer = _acquire();
				}
			}

		public:
			Writer() = default;
			explicit Writer(ByteStream &&stream);
			Writer(const Writer &) = delete;
			Writer &operator=(const Writer &) = delete;
			Writer(Writer &&) noexcept = default;
			Writer &operator=(Writer &&) noexcept = default;

			void clear() noexcept
			{
				if (_buffer)
				{
					_buffer->clear();
				}
			}
			[[nodiscard]] std::size_t size() const noexcept
			{
				return _buffer ? _buffer->size() : 0;
			}
			[[nodiscard]] std::span<const std::byte> data() const noexcept
			{
				return _buffer ? std::span<const std::byte>(*_buffer) : std::span<const std::byte>{};
			}

			void append(const void *source, std::size_t count)
			{
				if (count > Buffer{}.max_size() - size())
				{
					throw spk::Exception("ByteStream buffer size overflow.");
				}
				if (count == 0)
				{
					return;
				}
				_ensureBuffer();
				const auto *bytes = static_cast<const std::byte *>(source);
				_buffer->insert(_buffer->end(), bytes, bytes + count);
			}

			template <typename TValue>
				requires(std::is_trivially_copyable_v<TValue> && !std::is_pointer_v<TValue> && !std::is_member_pointer_v<TValue> && !std::is_array_v<TValue> && !std::same_as<TValue, std::string_view>)
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

			template <typename TCollection>
				requires ByteStreamCollectionSerializable<TCollection, Writer, Reader>
			Writer &operator<<(const TCollection &value)
			{
				if (value.size() > std::numeric_limits<std::uint32_t>::max())
				{
					throw spk::Exception("ByteStream collection exceeds maximum element count.");
				}
				using Element = typename TCollection::value_type;
				constexpr bool bulk = !ByteStreamMap<TCollection> &&
					std::ranges::contiguous_range<const TCollection> &&
					std::is_trivially_copyable_v<Element> &&
					!std::same_as<Element, bool>;
				if constexpr (bulk)
				{
					if (value.size() > Buffer{}.max_size() / sizeof(Element))
					{
						throw spk::Exception("ByteStream collection exceeds maximum byte count.");
					}
					*this << static_cast<std::uint32_t>(value.size());
					append(std::ranges::data(value), value.size() * sizeof(Element));
				}
				else
				{
					*this << static_cast<std::uint32_t>(value.size());
					for (const auto &element : value)
					{
						_writeCollectionElement<TCollection>(element);
					}
				}
				return *this;
			}

		private:
			template <typename TCollection>
			void _writeCollectionElement(const typename TCollection::value_type &element)
			{
				if constexpr (ByteStreamMap<TCollection>)
				{
					*this << element.first << element.second;
				}
				else if constexpr (std::same_as<typename TCollection::value_type, bool>)
				{
					*this << static_cast<bool>(element);
				}
				else
				{
					*this << element;
				}
			}

		public:

			void resize(std::size_t size)
			{
				if (size != 0)
				{
					_ensureBuffer();
				}
				if (_buffer)
				{
					_buffer->resize(size);
				}
			}

			void edit(std::size_t offset, const void *source, std::size_t count)
			{
				if (offset > size() || count > size() - offset)
				{
					throw spk::Exception("ByteStream edit outside bounds.");
				}
				if (count != 0)
				{
					std::memcpy(_buffer->data() + offset, source, count);
				}
			}

			[[nodiscard]] std::size_t capacity() const noexcept
			{
				return _buffer ? _buffer->capacity() : 0;
			}

			[[nodiscard]] std::span<std::byte> writableData() noexcept
			{
				return _buffer ? std::span<std::byte>(*_buffer) : std::span<std::byte>{};
			}

			[[nodiscard]] ByteStream build() &&
			{
				if (!_buffer)
				{
					return ByteStream();
				}
				return ByteStream(std::move(_buffer), true);
			}
		};

	private:
		std::shared_ptr<const Buffer> _buffer;
		bool _recyclable = false;
		explicit ByteStream(std::shared_ptr<const Buffer> buffer, bool recyclable = false) :
			_buffer(std::move(buffer)),
			_recyclable(recyclable)
		{
		}

	public:
		ByteStream() :
			_buffer(std::make_shared<const Buffer>())
		{
		}

		template <typename TValue>
			requires ByteStreamSerializable<TValue, Writer, Reader>
		explicit ByteStream(const TValue &value)
		{
			Writer writer;
			writer << value;
			*this = std::move(writer).build();
		}

		explicit ByteStream(std::string_view value)
		{
			Writer writer;
			writer << value;
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
			requires ByteStreamSerializable<TValue, Writer, Reader>
		[[nodiscard]] TValue cast() const
		{
			TValue result{};
			auto reader = this->reader();
			reader >> result;
			return result;
		}

		[[nodiscard]] Reader reader(std::size_t begin, std::size_t end) const
		{
			if (begin > end || end > size())
			{
				throw spk::Exception("Invalid ByteStream reader bounds.");
			}
			return Reader(_buffer, begin, end);
		}

		[[nodiscard]] Reader reader() const
		{
			return reader(0, size());
		}
	};
}

namespace spk
{
	inline ByteStream::Writer::Writer(ByteStream &&stream)
	{
		if (stream.empty())
		{
			return;
		}
		if (stream._recyclable && stream._buffer.use_count() == 1)
		{
			_buffer = std::const_pointer_cast<Buffer>(std::move(stream._buffer));
			return;
		}
		_buffer = _acquire();
		_buffer->assign(stream.data().begin(), stream.data().end());
	}
}
