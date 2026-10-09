#include "container/byte_stream.hpp"
#include "exception.hpp"
#include "network/message.hpp"

#include <cstdint>
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
