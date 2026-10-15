#include "engine/message_serializable_trait.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>

namespace
{
	class SerializableValue : public spk::MessageSerializableTrait
	{
	public:
		std::uint32_t value = 0;
		bool fail = false;
		bool recovered = false;

	protected:
		void _encode(spk::ByteStream::Writer &writer) const override
		{
			writer << value;
		}

		void _decode(const spk::ByteStream::Reader &reader) override
		{
			reader >> value;
			if (fail)
			{
				throw std::runtime_error("Test decode failure.");
			}
		}

		void _afterDecode(bool success) override
		{
			recovered = !success;
		}
	};

	class NestedValue final : public spk::MessageSerializableTrait
	{
	public:
		SerializableValue first;
		SerializableValue second;

	protected:
		void _encode(spk::ByteStream::Writer &writer) const override
		{
			first.encode(writer);
			second.encode(writer);
		}

		void _decode(const spk::ByteStream::Reader &reader) override
		{
			first.decode(reader);
			second.decode(reader);
		}
	};
}

TEST(MessageSerializableTraitTest, EncodesNestedObjectsAndConsumesExactBounds)
{
	NestedValue written;
	written.first.value = 12;
	written.second.value = 34;
	spk::ByteStream::Writer writer;
	written.encode(writer);
	auto reader = std::move(writer).build().reader();
	NestedValue received;
	received.decode(reader);
	EXPECT_EQ(received.first.value, 12u);
	EXPECT_EQ(received.second.value, 34u);
	EXPECT_EQ(reader.remaining(), 0u);
}

TEST(MessageSerializableTraitTest, FailedObjectDoesNotConsumeNextObject)
{
	SerializableValue first;
	first.value = 12;
	SerializableValue second;
	second.value = 34;
	spk::ByteStream::Writer writer;
	first.encode(writer);
	second.encode(writer);
	auto stream = std::move(writer).build();
	auto reader = stream.reader();
	SerializableValue received;
	received.fail = true;
	EXPECT_THROW(received.decode(reader), std::runtime_error);
	EXPECT_TRUE(received.recovered);
	received.fail = false;
	received.decode(reader);
	EXPECT_EQ(received.value, 34u);
	EXPECT_EQ(reader.remaining(), 0u);
}

TEST(MessageSerializableTraitTest, InvalidLengthRestoresParentReaderPosition)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{100};
	writer << std::uint8_t{1};
	auto stream = std::move(writer).build();
	auto reader = stream.reader();
	SerializableValue received;
	EXPECT_THROW(received.decode(reader), spk::Exception);
	EXPECT_EQ(reader.readOffset(), 0u);
}

TEST(MessageSerializableTraitTest, TrailingBytesFailOnlyCurrentFrame)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{8};
	writer << std::uint32_t{12};
	writer << std::uint32_t{99};
	SerializableValue good;
	good.value = 34;
	good.encode(writer);
	auto stream = std::move(writer).build();
	auto reader = stream.reader();
	SerializableValue received;
	EXPECT_THROW(received.decode(reader), spk::Exception);
	EXPECT_TRUE(received.recovered);
	received.decode(reader);
	EXPECT_EQ(received.value, 34u);
}
