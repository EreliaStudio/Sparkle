#include "container/byte_stream.hpp"
#include "exception.hpp"
#include "network/message.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <string>
#include <type_traits>
#include <utility>

namespace
{
	template <typename T>
	concept WritableReader = requires(T reader, std::uint32_t number) {
		reader << number;
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

TEST(ByteStream, ReaderRetainsStorageAfterOwnerDestruction)
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

TEST(ByteStream, SubreadersHaveIndependentRelativeCursors)
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

TEST(ByteStream, RefusesReadsOutsideReaderWithoutAdvancingCursor)
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

TEST(ByteStream, ReaderIsNotWritable)
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

TEST(ByteStream, MessageReadersOutliveMessage)
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

TEST(ByteStream, EmptyReaderIsSafe)
{
	spk::ByteStream stream;
	auto slice = stream.reader();
	EXPECT_EQ(slice.size(), 0u);
	EXPECT_EQ(slice.remaining(), 0u);
	EXPECT_THROW(slice.skip(1), spk::Exception);
}

TEST(ByteStream, ReadersShareBytesButNotReadPosition)
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

TEST(ByteStream, SerializeTrivialStructAsRawBytes)
{
	struct Value
	{
		std::uint32_t id;
		float x;
	};
	const Value original{42, 2.5f};
	spk::ByteStream::Writer writer;
	writer << original;
	const auto bytes = std::move(writer).build();
	auto reader = bytes.reader();
	Value decoded{};
	reader >> decoded;
	EXPECT_EQ(decoded.id, original.id);
	EXPECT_FLOAT_EQ(decoded.x, original.x);
	EXPECT_EQ(bytes.size(), sizeof(Value));
}

TEST(ByteStream, AppendPushAndEditMatchOriginalMessageAPI)
{
	spk::ByteStream::Writer writer;
	writer.append(std::uint32_t{7});
	writer.push(std::uint32_t{8});
	writer.edit(0, std::uint32_t{11});
	const auto bytes = std::move(writer).build();
	auto reader = bytes.reader();
	EXPECT_EQ(reader.get<std::uint32_t>(), 11u);
	EXPECT_EQ(reader.get<std::uint32_t>(), 8u);
}

TEST(ByteStream, ReadAtAndPeekDoNotAdvanceCursor)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{10} << std::uint32_t{20};
	const auto bytes = std::move(writer).build();
	auto reader = bytes.reader();
	EXPECT_EQ(reader.peek<std::uint32_t>(), 10u);
	EXPECT_EQ(reader.readAt<std::uint32_t>(4), 20u);
	EXPECT_EQ(reader.readOffset(), 0u);
	EXPECT_EQ(reader.get<std::uint32_t>(), 10u);
	EXPECT_EQ(reader.readOffset(), 4u);
}

TEST(ByteStream, StringUsesOriginalMessageWireFormat)
{
	const std::string source{"abc\0def", 7};
	spk::ByteStream::Writer writer;
	writer << source;
	const auto bytes = std::move(writer).build();
	auto reader = bytes.reader();
	EXPECT_EQ(reader.get<std::uint32_t>(), 7u);
	reader.reset();
	std::string result;
	reader >> result;
	EXPECT_EQ(result, source);
	EXPECT_EQ(reader.remaining(), 0u);
}

TEST(ByteStream, InvalidStringLengthDoesNotAllocate)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{1000} << std::uint16_t{9};
	const auto bytes = std::move(writer).build();
	auto reader = bytes.reader();
	std::string result("unchanged");
	EXPECT_THROW(reader >> result, spk::Exception);
	EXPECT_EQ(result, "unchanged");
}

TEST(ByteStream, RawAppendAndWritableData)
{
	const std::array<std::byte, 4> bytes{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
	spk::ByteStream::Writer writer;
	writer.append(bytes.data(), bytes.size());
	writer.append(nullptr, 0);
	writer.writableData()[0] = std::byte{42};
	const auto stored = std::move(writer).build();
	EXPECT_EQ(stored.data()[0], std::byte{42});
	EXPECT_EQ(stored.size(), bytes.size());
}

TEST(ByteStream, TruncatedReadDoesNotAdvanceReader)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{1};
	const auto bytes = std::move(writer).build();
	auto reader = bytes.reader();
	EXPECT_THROW((void)reader.get<std::uint64_t>(), spk::Exception);
	EXPECT_EQ(reader.readOffset(), 0u);
}

TEST(ByteStream, UniqueWriterReusesBufferAfterRebuild)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{12};
	auto stream = std::move(writer).build();
	const auto *original = stream.data().data();
	spk::ByteStream::Writer rebuild(std::move(stream));
	EXPECT_EQ(rebuild.data().data(), original);
	rebuild.edit(0, std::uint32_t{34});
	const auto result = std::move(rebuild).build();
	EXPECT_EQ(result.reader().get<std::uint32_t>(), 34u);
}

TEST(ByteStream, SharedBufferUsesCopyOnWrite)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{12};
	auto stream = std::move(writer).build();
	const auto copy = stream;
	spk::ByteStream::Writer rebuild(std::move(stream));
	rebuild.edit(0, std::uint32_t{34});
	EXPECT_EQ(copy.reader().get<std::uint32_t>(), 12u);
	EXPECT_EQ(std::move(rebuild).build().reader().get<std::uint32_t>(), 34u);
}
