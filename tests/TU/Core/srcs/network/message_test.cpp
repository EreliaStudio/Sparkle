#include <gtest/gtest.h>

#include "exception.hpp"
#include "network/message.hpp"

#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

namespace
{
	struct Payload
	{
		std::uint32_t identifier = 0;
		float value = 0.0f;
		bool enabled = false;

		[[nodiscard]] bool operator==(const Payload &) const = default;
	};

	static_assert(std::is_trivially_copyable_v<Payload>);
	static_assert(std::is_default_constructible_v<spk::Message> == false);
	static_assert(std::is_copy_constructible_v<spk::Message>);
	static_assert(std::is_copy_constructible_v<spk::Message::Writer> == false);

	[[nodiscard]] spk::Message emptyMessage(spk::Message::Type type)
	{
		return spk::Message::Writer(type).build();
	}
}

TEST(MessageTest, WriterBuildsImmutableMessageMetadata)
{
	spk::Message::Writer writer(42);
	writer.setRequestID(0x123456789ABCDEF0ull);

	const spk::Message message = std::move(writer).build();

	EXPECT_EQ(message.type(), 42u);
	EXPECT_EQ(message.requestID(), 0x123456789ABCDEF0ull);
	EXPECT_TRUE(message.empty());
	EXPECT_EQ(message.size(), 0u);
}

TEST(MessageTest, WriterRoundTripsTriviallyCopyableValues)
{
	const Payload expected{17, 12.5f, true};
	spk::Message::Writer writer(1);
	writer << expected;

	const spk::Message message = std::move(writer).build();
	auto reader = message.reader();

	EXPECT_EQ(message.size(), sizeof(Payload));
	EXPECT_EQ(reader.get<Payload>(), expected);
}

TEST(MessageTest, WriterAndReaderStreamSeveralValuesInOrder)
{
	spk::Message::Writer writer;
	writer << std::uint32_t{14} << float{3.5f} << true;
	const spk::Message message = std::move(writer).build();

	auto reader = message.reader();
	std::uint32_t integer = 0;
	float decimal = 0.0f;
	bool boolean = false;
	reader >> integer >> decimal >> boolean;

	EXPECT_EQ(integer, 14u);
	EXPECT_FLOAT_EQ(decimal, 3.5f);
	EXPECT_TRUE(boolean);
}

TEST(MessageTest, StringUsesLogicalContentInsteadOfObjectRepresentation)
{
	const std::string expected = "portable network payload";
	spk::Message::Writer writer;
	writer << expected;
	const spk::Message message = std::move(writer).build();

	auto reader = message.reader();
	std::string actual;
	reader >> actual;

	EXPECT_EQ(actual, expected);
}

TEST(MessageTest, IndependentReadersKeepIndependentOffsets)
{
	spk::Message::Writer writer;
	writer << std::uint32_t{11} << std::uint32_t{22};
	const spk::Message message = std::move(writer).build();

	auto first = message.reader();
	auto second = message.reader();

	EXPECT_EQ(first.get<std::uint32_t>(), 11u);
	EXPECT_EQ(first.readOffset(), sizeof(std::uint32_t));
	EXPECT_EQ(second.readOffset(), 0u);
	EXPECT_EQ(second.get<std::uint32_t>(), 11u);
	EXPECT_EQ(second.get<std::uint32_t>(), 22u);
	EXPECT_EQ(first.get<std::uint32_t>(), 22u);
}

TEST(MessageTest, ReaderCanStartSeekAndResetIndependently)
{
	spk::Message::Writer writer;
	writer << std::uint32_t{11} << std::uint32_t{22};
	const spk::Message message = std::move(writer).build();

	auto reader = message.reader(sizeof(std::uint32_t));
	EXPECT_EQ(reader.get<std::uint32_t>(), 22u);

	reader.reset();
	EXPECT_EQ(reader.get<std::uint32_t>(), 11u);

	reader.seek(sizeof(std::uint32_t));
	EXPECT_EQ(reader.get<std::uint32_t>(), 22u);
}

TEST(MessageTest, ReaderReadAtDoesNotAdvanceCursor)
{
	spk::Message::Writer writer;
	writer << std::uint32_t{11} << std::uint32_t{22};
	const spk::Message message = std::move(writer).build();

	auto reader = message.reader(sizeof(std::uint32_t));

	EXPECT_EQ(reader.readAt<std::uint32_t>(0), 11u);
	EXPECT_EQ(reader.readOffset(), sizeof(std::uint32_t));

	std::uint32_t value = 0;
	reader.readAt(sizeof(std::uint32_t), &value, sizeof(value));
	EXPECT_EQ(value, 22u);
	EXPECT_EQ(reader.readOffset(), sizeof(std::uint32_t));
}

TEST(MessageTest, ReaderBoundsChecksPreserveOffset)
{
	spk::Message::Writer writer;
	writer << std::uint16_t{7};
	const spk::Message message = std::move(writer).build();

	auto reader = message.reader();
	EXPECT_THROW((void)reader.peek<std::uint64_t>(), spk::Exception);
	EXPECT_THROW(reader.skip(sizeof(std::uint64_t)), spk::Exception);
	EXPECT_THROW((void)reader.readAt<std::uint32_t>(0), spk::Exception);
	EXPECT_EQ(reader.readOffset(), 0u);
}

TEST(MessageTest, WriterEditReplacesBytesBeforeBuild)
{
	spk::Message::Writer writer;
	writer << std::uint32_t{12};
	writer.edit(0, std::uint32_t{29});

	const spk::Message message = std::move(writer).build();

	EXPECT_EQ(message.reader().get<std::uint32_t>(), 29u);
}

TEST(MessageTest, WriterGrowthPreservesAlreadyWrittenBytes)
{
	spk::Message::Writer writer;
	writer << std::uint32_t{11};

	const std::size_t firstCapacity = writer.capacity();
	while (writer.capacity() == firstCapacity)
	{
		writer << std::uint32_t{22};
	}

	const spk::Message message = std::move(writer).build();
	EXPECT_EQ(message.reader().get<std::uint32_t>(), 11u);
}

TEST(MessageTest, ReaderKeepsStorageAliveAfterMessageDestruction)
{
	auto reader = [] {
		spk::Message::Writer writer;
		writer << std::uint32_t{73};
		const spk::Message message = std::move(writer).build();
		return message.reader();
	}();

	EXPECT_EQ(reader.get<std::uint32_t>(), 73u);
}

TEST(MessageTest, MessageCopiesShareImmutablePayloadStorage)
{
	spk::Message::Writer writer;
	writer << std::uint32_t{31};
	const spk::Message original = std::move(writer).build();
	const spk::Message copy = original;

	ASSERT_FALSE(original.empty());
	ASSERT_FALSE(copy.empty());
	EXPECT_EQ(original.data().data(), copy.data().data());
	EXPECT_EQ(copy.reader().get<std::uint32_t>(), 31u);
}

TEST(MessageTest, PooledStorageIsReusedAfterLastOwnerIsDestroyed)
{
	const std::byte *firstAddress = nullptr;

	{
		spk::Message::Writer writer;
		writer.resize(70);
		firstAddress = writer.data().data();
		const spk::Message message = std::move(writer).build();
		ASSERT_EQ(message.size(), 70u);
	}

	spk::Message::Writer writer;
	writer.resize(70);

	EXPECT_EQ(writer.data().data(), firstAddress);
}

TEST(MessageTest, EmptyMessageReaderRejectsDataReads)
{
	const spk::Message message = emptyMessage(7);
	auto reader = message.reader();

	EXPECT_TRUE(reader.empty());
	EXPECT_NO_THROW(reader.readAt(0, nullptr, 0));
	EXPECT_THROW((void)reader.get<std::uint32_t>(), spk::Exception);
}
