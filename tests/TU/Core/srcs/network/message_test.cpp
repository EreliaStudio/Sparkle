#include <gtest/gtest.h>

#include "exception.hpp"
#include "network/message.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

namespace
{
	const spk::Message staticMessage = [] {
		spk::Message::Writer writer(42);
		writer.setRequestID(91);
		writer << std::uint32_t{17};
		return std::move(writer).build();
	}();

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

TEST(MessageTest, MessageCanBeBuiltBeforeMain)
{
	EXPECT_EQ(staticMessage.type(), 42u);
	EXPECT_EQ(staticMessage.requestID(), 91u);
	EXPECT_EQ(staticMessage.reader().get<std::uint32_t>(), 17u);
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

TEST(MessageTest, WriterRebuildTakesUniqueMessageStorageWithoutCopy)
{
	spk::Message::Writer sourceWriter(52);
	sourceWriter.setRequestID(91u);
	sourceWriter << std::uint32_t{17};
	spk::Message source = std::move(sourceWriter).build();

	const std::byte *sourceAddress = source.data().data();

	spk::Message::Writer writer(std::move(source));

	EXPECT_EQ(writer.type(), 52u);
	EXPECT_EQ(writer.requestID(), 91u);
	ASSERT_EQ(writer.size(), sizeof(std::uint32_t));
	EXPECT_EQ(writer.data().data(), sourceAddress);
	EXPECT_EQ(
		std::bit_cast<std::uint32_t>(
			std::array<std::byte, sizeof(std::uint32_t)>{
				writer.data()[0],
				writer.data()[1],
				writer.data()[2],
				writer.data()[3]}),
		17u);

	writer.edit(0u, std::uint32_t{29});
	const spk::Message rebuilt = std::move(writer).build();

	EXPECT_EQ(rebuilt.type(), 52u);
	EXPECT_EQ(rebuilt.requestID(), 91u);
	EXPECT_EQ(rebuilt.data().data(), sourceAddress);
	EXPECT_EQ(rebuilt.reader().get<std::uint32_t>(), 29u);
}

TEST(MessageTest, WriterRebuildCopiesSharedMessageStorageBeforeEditing)
{
	spk::Message::Writer sourceWriter(53);
	sourceWriter.setRequestID(92u);
	sourceWriter << std::uint32_t{31};
	spk::Message source = std::move(sourceWriter).build();
	const spk::Message shared = source;

	const std::byte *sharedAddress = shared.data().data();

	spk::Message::Writer writer(std::move(source));

	ASSERT_EQ(writer.size(), sizeof(std::uint32_t));
	EXPECT_NE(writer.data().data(), sharedAddress);
	EXPECT_EQ(shared.reader().get<std::uint32_t>(), 31u);

	writer.edit(0u, std::uint32_t{47});
	const spk::Message rebuilt = std::move(writer).build();

	EXPECT_EQ(rebuilt.type(), 53u);
	EXPECT_EQ(rebuilt.requestID(), 92u);
	EXPECT_EQ(rebuilt.reader().get<std::uint32_t>(), 47u);
	EXPECT_EQ(shared.reader().get<std::uint32_t>(), 31u);
}

TEST(MessageTest, WriterRebuildPreservesLiveReaderStorage)
{
	spk::Message::Writer sourceWriter(54);
	sourceWriter << std::uint32_t{73};
	spk::Message source = std::move(sourceWriter).build();
	auto reader = source.reader();

	const std::byte *readerAddress = reader.data().data();

	spk::Message::Writer writer(std::move(source));

	EXPECT_NE(writer.data().data(), readerAddress);
	writer.edit(0u, std::uint32_t{99});
	const spk::Message rebuilt = std::move(writer).build();

	EXPECT_EQ(reader.get<std::uint32_t>(), 73u);
	EXPECT_EQ(rebuilt.reader().get<std::uint32_t>(), 99u);
}

TEST(MessageTest, WriterRebuildPreservesEmptyMessageMetadata)
{
	spk::Message::Writer sourceWriter(55);
	sourceWriter.setRequestID(93u);
	spk::Message source = std::move(sourceWriter).build();

	spk::Message::Writer writer(std::move(source));

	EXPECT_EQ(writer.type(), 55u);
	EXPECT_EQ(writer.requestID(), 93u);
	EXPECT_TRUE(writer.empty());
	EXPECT_EQ(writer.capacity(), 0u);

	const spk::Message rebuilt = std::move(writer).build();

	EXPECT_EQ(rebuilt.type(), 55u);
	EXPECT_EQ(rebuilt.requestID(), 93u);
	EXPECT_TRUE(rebuilt.empty());
}
