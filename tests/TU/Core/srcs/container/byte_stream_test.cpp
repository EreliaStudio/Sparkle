#include "container/byte_stream.hpp"
#include "exception.hpp"
#include "network/message.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <cstring>
#include <gtest/gtest.h>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace
{
	template <typename T>
	concept WritableReader = requires(T slice, std::uint32_t number) {
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
	spk::ByteStream::Reader slice = [] {
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
	auto parent = bytes.reader(4, 12);
	auto first = parent.subreader(0, 4);
	auto second = parent.subreader(4, 8);
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
	EXPECT_THROW(bytes.reader(3, 2), spk::Exception);
	EXPECT_THROW(bytes.reader(0, 5), spk::Exception);
	auto reader = bytes.reader();
	EXPECT_THROW(reader.subreader(4, 5), spk::Exception);
}

TEST(ByteStream, SliceIsNotWritable)
{
	static_assert(!WritableReader<spk::ByteStream::Reader>);
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
	spk::ByteStream::Reader slice = [] {
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

	struct TrivialVectorPayload
	{
		std::array<float, 4> position;
		std::array<float, 4> rotation;
		std::size_t id;
	};

	struct EncodedTrivialState
	{
		std::uint32_t value = 0;

		friend spk::ByteStream::Writer &operator<<(spk::ByteStream::Writer &writer, const EncodedTrivialState &state)
		{
			return writer << (state.value ^ 0xA5A5A5A5u);
		}

		friend const spk::ByteStream::Reader &operator>>(const spk::ByteStream::Reader &reader, EncodedTrivialState &state)
		{
			reader >> state.value;
			state.value ^= 0xA5A5A5A5u;
			return reader;
		}
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

		friend const spk::ByteStream::Reader &operator>>(const spk::ByteStream::Reader &reader, SerializableState &state)
		{
			return reader >> state.name >> state.health;
		}
	};

	static_assert(spk::ByteStreamSerializable<TrivialState, spk::ByteStream::Writer, spk::ByteStream::Reader>);
	static_assert(spk::ByteStreamSerializable<SerializableState, spk::ByteStream::Writer, spk::ByteStream::Reader>);
	static_assert(!spk::ByteStreamSerializable<NonTrivialState, spk::ByteStream::Writer, spk::ByteStream::Reader>);
	static_assert(!spk::ByteStreamSerializable<std::uint32_t *, spk::ByteStream::Writer, spk::ByteStream::Reader>);
	static_assert(std::is_trivially_copyable_v<TrivialState>);
	static_assert(std::is_trivially_copyable_v<TrivialVectorPayload>);
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
	std::uint32_t nameLength = 0;
	bytes.reader() >> nameLength;
	EXPECT_EQ(nameLength, original.name.size());
	EXPECT_EQ(bytes.size(), sizeof(nameLength) + original.name.size() + sizeof(original.health));
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

TEST(ByteStream, NonTrivialCastIgnoresBytesAfterDecodedObject)
{
	const SerializableState source{"first", 5};
	spk::ByteStream::Writer writer;
	writer << source << std::uint64_t{987};
	const auto bytes = std::move(writer).build();
	const auto decoded = bytes.cast<SerializableState>();
	EXPECT_EQ(decoded.name, "first");
	EXPECT_EQ(decoded.health, 5u);
}

TEST(ByteStream, NonTrivialCastRejectsStringOutsideBounds)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{50} << std::uint16_t{1};
	const auto bytes = std::move(writer).build();
	EXPECT_THROW((void)bytes.cast<SerializableState>(), spk::Exception);
}

TEST(ByteStream, NonTrivialCastRejectsMissingScalar)
{
	spk::ByteStream::Writer writer;
	writer << std::string("Alice");
	const auto bytes = std::move(writer).build();
	EXPECT_THROW((void)bytes.cast<SerializableState>(), spk::Exception);
}

TEST(ByteStream, TrivialVectorUsesElementCountAndRawBytes)
{
	const std::vector<std::uint16_t> source{2, 4, 6, 8};
	const spk::ByteStream encoded(source);
	const auto decoded = encoded.cast<std::vector<std::uint16_t>>();
	EXPECT_EQ(decoded, source);
	std::uint32_t count = 0;
	encoded.reader() >> count;
	EXPECT_EQ(count, source.size());
	EXPECT_EQ(encoded.size(), sizeof(count) + source.size() * sizeof(std::uint16_t));
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
	writer << std::uint32_t{2} << std::uint16_t{7};
	const auto truncated = std::move(writer).build();
	EXPECT_THROW((void)truncated.cast<std::vector<std::uint16_t>>(), spk::Exception);
}

TEST(ByteStream, StringVectorCastRejectsImpossibleElementCount)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{100000};
	const auto truncated = std::move(writer).build();
	EXPECT_THROW((void)truncated.cast<std::vector<std::string>>(), spk::Exception);
}

TEST(ByteStream, TypedConstructorMatchesWriterForComplexObject)
{
	const SerializableState original{"Bob", 17};
	spk::ByteStream::Writer writer;
	writer << original;
	const auto expected = std::move(writer).build();
	const spk::ByteStream actual(original);
	ASSERT_EQ(actual.size(), expected.size());
	EXPECT_EQ(std::memcmp(actual.data().data(), expected.data().data(), actual.size()), 0);
	EXPECT_EQ(actual.cast<SerializableState>().name, "Bob");
	EXPECT_EQ(actual.cast<SerializableState>().health, 17u);
}

TEST(ByteStream, VectorOfCustomObjectsUsesEachElementCodec)
{
	const std::vector<SerializableState> input{{"A", 10}, {"Longer", 20}};
	const spk::ByteStream bytes(input);
	const auto decoded = bytes.cast<std::vector<SerializableState>>();
	ASSERT_EQ(decoded.size(), input.size());
	EXPECT_EQ(decoded[0].name, "A");
	EXPECT_EQ(decoded[0].health, 10u);
	EXPECT_EQ(decoded[1].name, "Longer");
	EXPECT_EQ(decoded[1].health, 20u);
}

TEST(ByteStream, NestedVectorsRoundTrip)
{
	const std::vector<std::vector<std::uint16_t>> input{{1, 2}, {}, {3}};
	const spk::ByteStream bytes(input);
	EXPECT_EQ(bytes.cast<std::vector<std::vector<std::uint16_t>>>(), input);
}

TEST(ByteStream, PackedBooleanVectorRoundTrip)
{
	const std::vector<bool> input{true, false, false, true};
	const spk::ByteStream bytes(input);
	EXPECT_EQ(bytes.cast<std::vector<bool>>(), input);
}

TEST(ByteStream, ComplexValuesInWriterCanBeDecodedSequentially)
{
	spk::ByteStream::Writer writer;
	writer << SerializableState{"first", 1} << SerializableState{"second", 2};
	const auto bytes = std::move(writer).build();
	auto reader = bytes.reader();
	SerializableState first;
	SerializableState second;
	reader >> first >> second;
	EXPECT_EQ(reader.remaining(), 0u);
	EXPECT_EQ(first.name, "first");
	EXPECT_EQ(second.name, "second");
	EXPECT_EQ(second.health, 2u);
}

TEST(ByteStream, WriterAppendCopiesRawByteSpan)
{
	const std::array<std::byte, 4> source{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
	spk::ByteStream::Writer writer;
	writer.append(source.data(), source.size());
	writer.append(nullptr, 0);
	const auto stream = std::move(writer).build();
	ASSERT_EQ(stream.size(), source.size());
	EXPECT_EQ(std::memcmp(stream.data().data(), source.data(), source.size()), 0);
}

TEST(ByteStream, PrimitiveVectorWireFormatMatchesElementWiseSerialization)
{
	const std::vector<std::uint32_t> values{0, 1, 0xFEDCBA98u, 123456};
	const spk::ByteStream bulk(values);
	spk::ByteStream::Writer manual;
	manual << static_cast<std::uint32_t>(values.size());
	for (const auto value : values)
	{
		manual << value;
	}
	const auto expected = std::move(manual).build();
	ASSERT_EQ(bulk.size(), expected.size());
	EXPECT_EQ(std::memcmp(bulk.data().data(), expected.data().data(), bulk.size()), 0);
	EXPECT_EQ(bulk.cast<std::vector<std::uint32_t>>(), values);
}

TEST(ByteStream, BulkVectorHandlesEmptyAndLargeArrays)
{
	const std::vector<float> empty;
	const spk::ByteStream emptyBytes(empty);
	EXPECT_EQ(emptyBytes.size(), sizeof(std::uint32_t));
	EXPECT_EQ(emptyBytes.cast<std::vector<float>>(), empty);
	const std::vector<float> values(4096, 3.5f);
	const spk::ByteStream encoded(values);
	EXPECT_EQ(encoded.size(), sizeof(std::uint32_t) + values.size() * sizeof(float));
	EXPECT_EQ(encoded.cast<std::vector<float>>(), values);
}

TEST(ByteStream, ByteVectorIsWrittenAsContiguousPayload)
{
	const std::vector<std::byte> bytes{std::byte{0}, std::byte{1}, std::byte{255}};
	const spk::ByteStream stream(bytes);
	ASSERT_EQ(stream.size(), sizeof(std::uint32_t) + bytes.size());
	EXPECT_EQ(std::memcmp(stream.data().data() + sizeof(std::uint32_t), bytes.data(), bytes.size()), 0);
	EXPECT_EQ(stream.cast<std::vector<std::byte>>(), bytes);
}

TEST(ByteStream, TrivialStructWithCustomCodecUsesRawVectorBytes)
{
	static_assert(std::is_trivially_copyable_v<EncodedTrivialState>);
	const std::vector<EncodedTrivialState> values{{1}, {2}, {100}};
	const spk::ByteStream encoded(values);
	const std::size_t bytes = values.size() * sizeof(EncodedTrivialState);
	ASSERT_EQ(encoded.size(), sizeof(std::uint32_t) + bytes);
	EXPECT_EQ(std::memcmp(encoded.data().data() + sizeof(std::uint32_t), values.data(), bytes), 0);
	spk::ByteStream::Writer writer;
	writer << static_cast<std::uint32_t>(values.size());
	for (const auto &value : values)
	{
		writer << value;
	}
	const auto elementEncoded = std::move(writer).build();
	EXPECT_NE(std::memcmp(encoded.data().data(), elementEncoded.data().data(), encoded.size()), 0);
	const auto decoded = encoded.cast<std::vector<EncodedTrivialState>>();
	ASSERT_EQ(decoded.size(), values.size());
	for (std::size_t index = 0; index < values.size(); ++index)
	{
		EXPECT_EQ(decoded[index].value, values[index].value);
	}
}

TEST(ByteStream, TrivialStructVectorCopiesContiguousObjects)
{
	const std::vector<TrivialVectorPayload> values{
		{{1, 2, 3, 4}, {5, 6, 7, 8}, 9},
		{{10, 20, 30, 40}, {50, 60, 70, 80}, 90},
		{{0, 0, 0, 0}, {-1, -2, -3, -4}, 12345}};
	const spk::ByteStream encoded(values);
	const std::size_t payloadSize = values.size() * sizeof(TrivialVectorPayload);
	ASSERT_EQ(encoded.size(), sizeof(std::uint32_t) + payloadSize);
	std::uint32_t count = 0;
	encoded.reader() >> count;
	EXPECT_EQ(count, values.size());
	EXPECT_EQ(std::memcmp(encoded.data().data() + sizeof(count), values.data(), payloadSize), 0);
	const auto decoded = encoded.cast<std::vector<TrivialVectorPayload>>();
	ASSERT_EQ(decoded.size(), values.size());
	for (std::size_t index = 0; index < values.size(); ++index)
	{
		EXPECT_EQ(decoded[index].position, values[index].position);
		EXPECT_EQ(decoded[index].rotation, values[index].rotation);
		EXPECT_EQ(decoded[index].id, values[index].id);
	}
}

TEST(ByteStream, TrivialStructVectorRejectsTruncatedPayload)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{2} << TrivialVectorPayload{{1, 2, 3, 4}, {5, 6, 7, 8}, 9};
	const auto encoded = std::move(writer).build();
	EXPECT_THROW((void)encoded.cast<std::vector<TrivialVectorPayload>>(), spk::Exception);
}

TEST(ByteStream, TruncatedBulkReadPreservesTargetAndReadCursorAfterPrefix)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{3} << std::uint32_t{12} << std::uint32_t{34};
	const auto stream = std::move(writer).build();
	auto reader = stream.reader();
	std::vector<std::uint32_t> value{99};
	EXPECT_THROW(reader >> value, spk::Exception);
	EXPECT_EQ(value, (std::vector<std::uint32_t>{99}));
	EXPECT_EQ(reader.readOffset(), sizeof(std::uint32_t));
}

TEST(ByteStream, OrderedSetRoundTrip)
{
	const std::set<std::uint32_t> values{1, 3, 8, 13};
	const spk::ByteStream stream(values);
	EXPECT_EQ(stream.cast<std::set<std::uint32_t>>(), values);
	EXPECT_EQ(stream.size(), sizeof(std::uint32_t) + values.size() * sizeof(std::uint32_t));
}

TEST(ByteStream, ListAndDequeRoundTrip)
{
	const std::list<std::string> words{"alpha", "beta", ""};
	const std::deque<std::uint16_t> numbers{1, 2, 3, 4};
	spk::ByteStream::Writer writer;
	writer << words << numbers;
	const auto stream = std::move(writer).build();
	auto reader = stream.reader();
	std::list<std::string> decodedWords;
	std::deque<std::uint16_t> decodedNumbers;
	reader >> decodedWords >> decodedNumbers;
	EXPECT_EQ(decodedWords, words);
	EXPECT_EQ(decodedNumbers, numbers);
	EXPECT_EQ(reader.remaining(), 0u);
}

TEST(ByteStream, MultisetPreservesDuplicateElements)
{
	const std::multiset<int> values{2, 2, 5, 5, 7};
	const spk::ByteStream stream(values);
	EXPECT_EQ(stream.cast<std::multiset<int>>(), values);
}

TEST(ByteStream, UnorderedSetRoundTrip)
{
	const std::unordered_set<std::uint32_t> values{8, 1, 99, 3};
	const spk::ByteStream stream(values);
	EXPECT_EQ(stream.cast<std::unordered_set<std::uint32_t>>(), values);
}

TEST(ByteStream, OrderedMapRoundTrip)
{
	const std::map<std::string, std::uint32_t> scores{{"alice", 42}, {"bob", 17}};
	const spk::ByteStream stream(scores);
	EXPECT_EQ((stream.cast<std::map<std::string, std::uint32_t>>()), scores);
}

TEST(ByteStream, UnorderedMapWithNestedContainerRoundTrip)
{
	const std::unordered_map<std::string, std::vector<std::uint16_t>> values{
		{"north", {1, 2, 3}}, {"south", {8, 13}}};
	const spk::ByteStream stream(values);
	EXPECT_EQ((stream.cast<std::unordered_map<std::string, std::vector<std::uint16_t>>>()), values);
}

TEST(ByteStream, NestedNonContiguousCollectionsRoundTrip)
{
	const std::vector<std::set<std::uint16_t>> values{{3, 1}, {}, {7, 8}};
	const spk::ByteStream stream(values);
	EXPECT_EQ(stream.cast<std::vector<std::set<std::uint16_t>>>(), values);
}


TEST(ByteStream, GenericCollectionsHaveCompatibleElementEncoding)
{
	const std::vector<std::uint32_t> vector{1, 4, 7};
	const std::set<std::uint32_t> set{1, 4, 7};
	const spk::ByteStream a(vector);
	const spk::ByteStream b(set);
	EXPECT_EQ(a.size(), b.size());
	EXPECT_EQ(std::memcmp(a.data().data(), b.data().data(), a.size()), 0);
}

TEST(ByteStream, MalformedCollectionDoesNotModifyDestination)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{3} << std::uint32_t{42};
	const auto stream = std::move(writer).build();
	auto reader = stream.reader();
	std::set<std::uint32_t> existing{77};
	EXPECT_THROW(reader >> existing, spk::Exception);
	EXPECT_EQ(existing, (std::set<std::uint32_t>{77}));
}

TEST(ByteStream, InvalidCollectionCountFailsBeforeInsertion)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{1000000};
	const auto stream = std::move(writer).build();
	EXPECT_THROW((void)stream.cast<std::list<std::string>>(), spk::Exception);
	EXPECT_THROW(((void)stream.cast<std::map<std::string, std::uint32_t>>()), spk::Exception);
}

TEST(ByteStream, GenericCollectionConceptRejectsFixedAndTextRanges)
{
	static_assert(spk::ByteStreamCollection<std::vector<int>>);
	static_assert(spk::ByteStreamCollection<std::set<int>>);
	static_assert(spk::ByteStreamCollection<std::map<int, int>>);
	static_assert(!spk::ByteStreamCollection<std::array<int, 3>>);
	static_assert(!spk::ByteStreamCollection<std::string>);
	SUCCEED();
}
