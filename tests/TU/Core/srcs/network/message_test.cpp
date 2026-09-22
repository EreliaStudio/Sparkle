#include <gtest/gtest.h>

#include "exception.hpp"
#include "network/message.hpp"

#include <cstdint>
#include <string>
#include <type_traits>

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
}

TEST(MessageTest, StoresTypeAndStartsEmpty)
{
	const spk::Message message(42);

	EXPECT_EQ(message.type(), 42u);
	EXPECT_TRUE(message.empty());
	EXPECT_EQ(message.size(), 0u);
}

TEST(MessageTest, RoundTripsTriviallyCopyableValues)
{
	const Payload expected{17, 12.5f, true};
	spk::Message message(1);

	message << expected;

	EXPECT_EQ(message.size(), sizeof(Payload));
	EXPECT_EQ(message.get<Payload>(), expected);
}

TEST(MessageTest, StreamsSeveralValuesInInsertionOrder)
{
	spk::Message message;
	message << std::uint32_t{14} << float{3.5f} << true;

	std::uint32_t integer = 0;
	float decimal = 0.0f;
	bool boolean = false;
	message >> integer >> decimal >> boolean;

	EXPECT_EQ(integer, 14u);
	EXPECT_FLOAT_EQ(decimal, 3.5f);
	EXPECT_TRUE(boolean);
}

TEST(MessageTest, StringUsesLogicalContentInsteadOfObjectRepresentation)
{
	spk::Message message;
	const std::string expected = "portable network payload";

	message << expected;

	std::string actual;
	message >> actual;
	EXPECT_EQ(actual, expected);
}

TEST(MessageTest, PeekDoesNotAdvanceReadOffset)
{
	spk::Message message;
	message << std::uint32_t{73};

	EXPECT_EQ(message.peek<std::uint32_t>(), 73u);
	EXPECT_EQ(message.readOffset(), 0u);
	EXPECT_EQ(message.get<std::uint32_t>(), 73u);
	EXPECT_EQ(message.readOffset(), sizeof(std::uint32_t));
}

TEST(MessageTest, EditReplacesBytesInPlace)
{
	spk::Message message;
	message << std::uint32_t{12};
	message.edit(0, std::uint32_t{29});

	EXPECT_EQ(message.get<std::uint32_t>(), 29u);
}

TEST(MessageTest, ResetReturnsReaderToBeginning)
{
	spk::Message message;
	message << std::uint32_t{9};

	EXPECT_EQ(message.get<std::uint32_t>(), 9u);
	message.reset();
	EXPECT_EQ(message.get<std::uint32_t>(), 9u);
}

TEST(MessageTest, ReadingPastPayloadThrows)
{
	const spk::Message message;

	EXPECT_THROW((void)message.get<std::uint32_t>(), spk::Exception);
}
