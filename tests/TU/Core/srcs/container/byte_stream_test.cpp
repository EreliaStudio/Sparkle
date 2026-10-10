#include "container/byte_stream.hpp"
#include "exception.hpp"
#include "network/message.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <string>
#include <type_traits>
#include <utility>

namespace
{
	template <typename T>
	concept WritableSlice = requires(T slice, std::uint32_t number) {
		slice << number;
	};
}

TEST(ByteStream, SerializeScalarAndString)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{17} << std::string("example");
	const auto bytes = std::move(writer).build();
	auto reader = bytes.reader();
	std::uint32_t value = 0;
	std::string text;
	reader >> value >> text;
	EXPECT_EQ(value, 17u);
	EXPECT_EQ(text, "example");
	EXPECT_EQ(reader.remaining(), 0u);
}

TEST(ByteStream, SliceRetainsStorageAfterOwnerDestruction)
{
	spk::ByteStream::Slice slice = [] {
		spk::ByteStream::Writer writer;
		writer << std::uint32_t{42};
		return std::move(writer).build().reader();
	}();
	std::uint32_t value = 0;
	slice >> value;
	EXPECT_EQ(value, 42u);
}

TEST(ByteStream, NestedSlicesHaveIndependentRelativeCursors)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{1} << std::uint32_t{2} << std::uint32_t{3};
	const auto bytes = std::move(writer).build();
	auto parent = bytes.slice(4, 12);
	auto first = parent.slice(0, 4);
	auto second = parent.slice(4, 8);
	std::uint32_t a = 0;
	std::uint32_t b = 0;
	first >> a;
	second >> b;
	EXPECT_EQ(a, 2u);
	EXPECT_EQ(b, 3u);
	EXPECT_EQ(parent.readOffset(), 0u);
}

TEST(ByteStream, RefusesReadsOutsideSliceWithoutAdvancingCursor)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{1};
	auto bytes = std::move(writer).build();
	auto reader = bytes.reader();
	std::uint64_t value = 0;
	EXPECT_THROW(reader >> value, spk::Exception);
	EXPECT_EQ(reader.readOffset(), 0u);
	EXPECT_THROW(reader.skip(5), spk::Exception);
	EXPECT_THROW(reader.seek(5), spk::Exception);
}

TEST(ByteStream, RejectsInvalidParentAndNestedBounds)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{1};
	auto bytes = std::move(writer).build();
	EXPECT_THROW(bytes.slice(3, 2), spk::Exception);
	EXPECT_THROW(bytes.slice(0, 5), spk::Exception);
	auto reader = bytes.reader();
	EXPECT_THROW(reader.slice(4, 5), spk::Exception);
}

TEST(ByteStream, SliceIsNotWritable)
{
	static_assert(!WritableSlice<spk::ByteStream::Slice>);
	SUCCEED();
}

TEST(ByteStream, MessagePayloadSharesStorageWithoutCopying)
{
	spk::Message::Writer writer(12);
	writer << std::uint32_t{73};
	auto message = std::move(writer).build();
	auto bytes = message.payload();
	ASSERT_EQ(bytes.size(), message.size());
	EXPECT_EQ(bytes.data().data(), message.data().data());
	std::uint32_t value = 0;
	bytes.reader() >> value;
	EXPECT_EQ(value, 73u);
}

TEST(ByteStream, MessageSlicesOutliveMessage)
{
	spk::ByteStream::Slice slice = [] {
		spk::Message::Writer writer(12);
		writer << std::uint32_t{73};
		auto message = std::move(writer).build();
		return message.payload().reader();
	}();
	std::uint32_t value = 0;
	slice >> value;
	EXPECT_EQ(value, 73u);
}

TEST(ByteStream, EmptySliceIsSafe)
{
	spk::ByteStream stream;
	auto slice = stream.reader();
	EXPECT_EQ(slice.size(), 0u);
	EXPECT_EQ(slice.remaining(), 0u);
	EXPECT_THROW(slice.skip(1), spk::Exception);
}

TEST(ByteStream, SlicesShareBytesButNotReadPosition)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{7} << std::uint32_t{8};
	auto stream = std::move(writer).build();
	auto first = stream.reader();
	auto second = stream.reader();
	std::uint32_t value = 0;
	first >> value;
	EXPECT_EQ(value, 7u);
	EXPECT_EQ(first.readOffset(), 4u);
	EXPECT_EQ(second.readOffset(), 0u);
	second >> value;
	EXPECT_EQ(value, 7u);
}

TEST(ByteStream, CopiedByteStreamsShareTheSameImmutableBuffer)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{99};
	auto original = std::move(writer).build();
	auto copy = original;
	EXPECT_EQ(original.data().data(), copy.data().data());
}

namespace
{
	struct TrivialState
	{
		std::uint32_t id;
		float position;
		std::uint16_t flags;
	};

	struct NonTrivialState
	{
		std::string text;
	};

	struct SerializableState
	{
		std::string name;
		std::uint32_t health = 0;

		friend spk::ByteStream::Writer &operator<<(spk::ByteStream::Writer &writer, const SerializableState &state)
		{
			return writer << state.name << state.health;
		}

		friend const spk::ByteStream::Slice &operator>>(const spk::ByteStream::Slice &reader, SerializableState &state)
		{
			return reader >> state.name >> state.health;
		}
	};

	static_assert(std::is_trivially_copyable_v<TrivialState>);
	static_assert(std::is_constructible_v<spk::ByteStream, TrivialState>);
	static_assert(!std::is_constructible_v<spk::ByteStream, NonTrivialState>);
	static_assert(std::is_constructible_v<spk::ByteStream, SerializableState>);
	static_assert(std::is_constructible_v<spk::ByteStream, std::string>);
	static_assert(std::is_constructible_v<spk::ByteStream, std::string_view>);
	static_assert(!std::is_constructible_v<spk::ByteStream, std::uint32_t *>);
	static_assert(std::is_copy_constructible_v<spk::ByteStream>);
}

TEST(ByteStream, TypedConstructorPreservesTrivialStructRepresentation)
{
	const TrivialState original{42u, 8.5f, 7u};
	const spk::ByteStream stream(original);
	EXPECT_EQ(stream.size(), sizeof(TrivialState));
	EXPECT_EQ(std::memcmp(stream.data().data(), &original, sizeof(TrivialState)), 0);
	const auto restored = stream.cast<TrivialState>();
	EXPECT_EQ(restored.id, original.id);
	EXPECT_FLOAT_EQ(restored.position, original.position);
	EXPECT_EQ(restored.flags, original.flags);
}

TEST(ByteStream, TypedConstructorCopiesStorageRatherThanReferencingSource)
{
	TrivialState source{3u, 2.0f, 1u};
	const spk::ByteStream stream(source);
	source.id = 100u;
	EXPECT_EQ(stream.cast<TrivialState>().id, 3u);
}

TEST(ByteStream, CastAcceptsTrailingBytes)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{0x12345678} << std::uint64_t{99};
	const auto stream = std::move(writer).build();
	EXPECT_EQ(stream.size(), sizeof(std::uint32_t) + sizeof(std::uint64_t));
	EXPECT_EQ(stream.cast<std::uint32_t>(), 0x12345678u);
}

TEST(ByteStream, CastRejectsTooSmallAndEmptyBuffers)
{
	const spk::ByteStream stream(std::uint16_t{9});
	EXPECT_THROW((void)stream.cast<std::uint32_t>(), spk::Exception);
	const spk::ByteStream empty;
	EXPECT_THROW((void)empty.cast<std::uint8_t>(), spk::Exception);
}

TEST(ByteStream, CastDoesNotAdvanceIndependentSliceCursors)
{
	const spk::ByteStream stream(std::uint64_t{42});
	auto reader = stream.reader();
	EXPECT_EQ(stream.cast<std::uint64_t>(), 42u);
	EXPECT_EQ(reader.readOffset(), 0u);
	std::uint64_t value = 0;
	reader >> value;
	EXPECT_EQ(value, 42u);
}

TEST(ByteStream, TypedConstructorPreservesCopyAndMoveSemantics)
{
	const spk::ByteStream original(std::uint64_t{25});
	const spk::ByteStream shared(original);
	spk::ByteStream moved(shared);
	EXPECT_EQ(original.data().data(), moved.data().data());
	EXPECT_EQ(moved.cast<std::uint64_t>(), 25u);
}

TEST(ByteStream, StringConstructorMatchesMessageLengthPrefix)
{
	const std::string text{"Example\0NUL", 11};
	const spk::ByteStream value(std::string_view(text.data(), text.size()));
	auto reader = value.reader();
	std::uint32_t length = 0;
	reader >> length;
	EXPECT_EQ(length, text.size());
	std::string decoded;
	EXPECT_EQ(value.cast<std::string>(), text);
	reader.reset();
	reader >> decoded;
	EXPECT_EQ(decoded, text);
	EXPECT_EQ(reader.remaining(), 0u);
}

TEST(ByteStream, EmptyStringIsLengthPrefixed)
{
	const spk::ByteStream empty(std::string{});
	EXPECT_EQ(empty.size(), sizeof(std::uint32_t));
	EXPECT_EQ(empty.cast<std::string>(), "");
}

TEST(ByteStream, StringViewConstructorOwnsItsBytes)
{
	std::string source = "initial";
	const spk::ByteStream bytes{std::string_view(source)};
	source.assign("changed");
	EXPECT_EQ(bytes.cast<std::string>(), "initial");
}

TEST(ByteStream, StringCastAllowsTrailingBytes)
{
	spk::ByteStream::Writer writer;
	writer << std::string("first") << std::uint64_t{0x1234};
	const auto bytes = std::move(writer).build();
	EXPECT_EQ(bytes.cast<std::string>(), "first");
}

TEST(ByteStream, StringCastRejectsTruncatedAndOversizedLength)
{
	spk::ByteStream::Writer shortWriter;
	shortWriter << std::uint16_t{1};
	EXPECT_THROW((void)std::move(shortWriter).build().cast<std::string>(), spk::Exception);
	spk::ByteStream::Writer invalidWriter;
	invalidWriter << std::uint32_t{100u} << std::uint8_t{5};
	EXPECT_THROW((void)std::move(invalidWriter).build().cast<std::string>(), spk::Exception);
}

TEST(ByteStream, NonTrivialCodecSerializesStringAndScalar)
{
	const SerializableState original{"Alice", 42};
	const spk::ByteStream bytes(original);
	std::uint32_t payloadLength = 0;
	bytes.reader() >> payloadLength;
	EXPECT_EQ(payloadLength + sizeof(payloadLength), bytes.size());
	const auto decoded = bytes.cast<SerializableState>();
	EXPECT_EQ(decoded.name, original.name);
	EXPECT_EQ(decoded.health, original.health);
}

TEST(ByteStream, NonTrivialCodecRetainsIndependentStorage)
{
	spk::ByteStream bytes = [] {
		SerializableState source{"temporary", 77};
		return spk::ByteStream(source);
	}();
	EXPECT_EQ(bytes.cast<SerializableState>().name, "temporary");
	EXPECT_EQ(bytes.cast<SerializableState>().health, 77u);
}

TEST(ByteStream, NonTrivialCastIgnoresBytesFollowingFramedObject)
{
	const SerializableState source{"first", 5};
	spk::ByteStream::Writer payload;
	payload << source;
	spk::ByteStream::Writer outer;
	outer << static_cast<std::uint32_t>(payload.size());
	outer.append(payload.data().data(), payload.size());
	outer << std::uint64_t{987};
	const auto bytes = std::move(outer).build();
	const auto decoded = bytes.cast<SerializableState>();
	EXPECT_EQ(decoded.name, "first");
	EXPECT_EQ(decoded.health, 5u);
}

TEST(ByteStream, NonTrivialCastRejectsBrokenFrameBoundaries)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{50} << std::uint16_t{1};
	const auto bytes = std::move(writer).build();
	EXPECT_THROW((void)bytes.cast<SerializableState>(), spk::Exception);
}

TEST(ByteStream, NonTrivialCastRejectsTruncatedNestedString)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{sizeof(std::uint32_t)} << std::uint32_t{99};
	const auto bytes = std::move(writer).build();
	EXPECT_THROW((void)bytes.cast<SerializableState>(), spk::Exception);
}

TEST(ByteStream, TrivialVectorUsesElementCountAndRawBytes)
{
	const std::vector<std::uint16_t> source{2, 4, 6, 8};
	const spk::ByteStream encoded(source);
	const auto decoded = encoded.cast<std::vector<std::uint16_t>>();
	EXPECT_EQ(decoded, source);
	std::uint32_t payloadSize = 0;
	encoded.reader() >> payloadSize;
	EXPECT_EQ(payloadSize, sizeof(std::uint32_t) + source.size() * sizeof(std::uint16_t));
}

TEST(ByteStream, StringVectorPreservesDynamicLengthsAndEmbeddedNulls)
{
	const std::vector<std::string> source{"", std::string("ab\0c", 4), "longer value"};
	const spk::ByteStream encoded(source);
	EXPECT_EQ(encoded.cast<std::vector<std::string>>(), source);
}

TEST(ByteStream, VectorCastRejectsTruncatedData)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{sizeof(std::uint32_t) + sizeof(std::uint16_t)} << std::uint32_t{2} << std::uint16_t{7};
	const auto truncated = std::move(writer).build();
	EXPECT_THROW((void)truncated.cast<std::vector<std::uint16_t>>(), spk::Exception);
}

TEST(ByteStream, StringVectorCastRejectsImpossibleElementCount)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{sizeof(std::uint32_t)} << std::uint32_t{100000};
	const auto truncated = std::move(writer).build();
	EXPECT_THROW((void)truncated.cast<std::vector<std::string>>(), spk::Exception);
}
