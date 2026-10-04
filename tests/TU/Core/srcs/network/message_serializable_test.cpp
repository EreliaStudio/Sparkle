#include "network/message.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace
{
	struct CustomPayload
	{
		std::string text;
		friend spk::Message::Writer &operator<<(spk::Message::Writer &writer, const CustomPayload &value)
		{
			return writer << value.text;
		}
		friend const spk::Message::Reader &operator>>(const spk::Message::Reader &reader, CustomPayload &value)
		{
			return reader >> value.text;
		}
	};
	struct WriteOnly
	{
		std::string text;
		friend spk::Message::Writer &operator<<(spk::Message::Writer &writer, const WriteOnly &value)
		{
			return writer << value.text;
		}
	};
	struct ReadOnly
	{
		std::string text;
		friend const spk::Message::Reader &operator>>(const spk::Message::Reader &reader, ReadOnly &value)
		{
			return reader >> value.text;
		}
	};
	static_assert(spk::MessageSerializable<std::uint32_t>);
	static_assert(spk::MessageSerializable<std::string>);
	static_assert(spk::MessageSerializable<CustomPayload>);
	static_assert(spk::MessageSerializable<WriteOnly> == false);
	static_assert(spk::MessageSerializable<ReadOnly> == false);
	static_assert(spk::MessageSerializable<std::vector<std::string>> == false);
}

TEST(MessageSerializableTest, CustomOperatorsRoundTripThroughADL)
{
	const CustomPayload expected{"Custom payload"};
	spk::Message::Writer writer;
	writer << expected;
	const auto message = std::move(writer).build();
	CustomPayload actual;
	message.reader() >> actual;
	EXPECT_EQ(actual.text, expected.text);
}
