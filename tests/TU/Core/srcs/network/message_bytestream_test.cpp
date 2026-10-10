#include "container/byte_stream.hpp"
#include "network/message.hpp"

#include <cstdint>
#include <gtest/gtest.h>
#include <type_traits>
#include <utility>

namespace
{
	static_assert(std::is_same_v<decltype(std::declval<const spk::Message &>().payload()), const spk::ByteStream &>);
	static_assert(std::is_same_v<decltype(std::declval<spk::Message &>().payload()), spk::ByteStream &>);
	static_assert(std::is_same_v<decltype(std::declval<spk::Message::Writer &>().payload()), spk::ByteStream::Writer &>);
}

TEST(MessageByteStream, HeaderAndPayloadAreIndependent)
{
	spk::Message::Writer writer;
	writer.header().messageType = 0x1234;
	writer.header().requestID = 77;
	writer.payload() << std::uint32_t{42};
	const spk::Message message = std::move(writer).build();
	EXPECT_EQ(message.header().messageType, 0x1234u);
	EXPECT_EQ(message.header().requestID, 77u);
	EXPECT_EQ(message.payload().reader().get<std::uint32_t>(), 42u);
}

TEST(MessageByteStream, PayloadGetterReturnsOwnedByteStreamReference)
{
	spk::Message::Writer writer(12);
	writer.payload() << std::uint32_t{17};
	const spk::Message message = std::move(writer).build();
	const spk::ByteStream &first = message.payload();
	const spk::ByteStream &second = message.payload();
	EXPECT_EQ(&first, &second);
	EXPECT_EQ(first.data().data(), message.data().data());
}

TEST(MessageByteStream, UniqueRebuildReusesPooledStorage)
{
	spk::Message::Writer writer(12);
	writer.payload() << std::uint32_t{17};
	auto message = std::move(writer).build();
	const auto *original = message.payload().data().data();
	spk::Message::Writer rebuild(std::move(message));
	EXPECT_EQ(rebuild.payload().data().data(), original);
	rebuild.edit(0, std::uint32_t{31});
	const auto updated = std::move(rebuild).build();
	EXPECT_EQ(updated.payload().reader().get<std::uint32_t>(), 31u);
	EXPECT_EQ(updated.payload().data().data(), original);
}

TEST(MessageByteStream, SharedPayloadStaysImmutableWhenRebuilt)
{
	spk::Message::Writer writer(12);
	writer.payload() << std::uint32_t{17};
	auto message = std::move(writer).build();
	const auto copy = message;
	const auto *original = copy.payload().data().data();
	spk::Message::Writer rebuild(std::move(message));
	EXPECT_NE(rebuild.payload().data().data(), original);
	rebuild.edit(0, std::uint32_t{31});
	EXPECT_EQ(copy.payload().reader().get<std::uint32_t>(), 17u);
	EXPECT_EQ(std::move(rebuild).build().payload().reader().get<std::uint32_t>(), 31u);
}

TEST(MessageByteStream, MessageReaderAndByteStreamReaderAreSameType)
{
	static_assert(std::is_same_v<spk::Message::Reader, spk::ByteStream::Reader>);
	spk::Message::Writer writer(12);
	writer.payload() << std::uint32_t{4} << std::uint32_t{9};
	const auto message = std::move(writer).build();
	auto reader = message.reader(sizeof(std::uint32_t));
	EXPECT_EQ(reader.get<std::uint32_t>(), 9u);
	EXPECT_EQ(reader.readAt<std::uint32_t>(0), 4u);
	EXPECT_EQ(reader.readOffset(), 2 * sizeof(std::uint32_t));
}
