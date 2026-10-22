#include "design_pattern/publishable.hpp"
#include "exception.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
	class Publisher : public spk::Publishable
	{
	public:
		explicit Publisher(Mode mode = Mode::Source) :
			Publishable(mode)
		{
		}

		using Publishable::setPartRegistry;
	};

	class Probe : public spk::Publishable::Part
	{
	public:
		int value = 0;
		mutable int captured = 0;
		int decoded = 0;
		mutable std::function<void()> duringEncode;
		std::function<void()> duringDecode;

		void set(int next)
		{
			value = next;
			invalidate();
		}

	protected:
		void _encode(spk::ByteStream::Writer &writer) const override
		{
			++captured;
			if (duringEncode)
			{
				duringEncode();
			}
			writer << value;
		}

		void _decode(const spk::ByteStream::Reader &reader) override
		{
			if (duringDecode)
			{
				duringDecode();
			}
			reader >> value;
			++decoded;
		}
	};

	spk::ByteStream message(std::uint8_t mode, std::uint32_t id, std::uint64_t generation, int value)
	{
		spk::ByteStream::Writer writer;
		writer << mode << std::uint32_t{1} << id << generation << std::uint64_t{sizeof(int)} << value;
		return std::move(writer).build();
	}

	std::vector<std::uint32_t> ids(const spk::ByteStream &stream)
	{
		std::vector<std::uint32_t> result;
		if (stream.empty())
		{
			return result;
		}
		const auto reader = stream.reader();
		std::uint8_t mode;
		std::uint32_t count;
		reader >> mode >> count;
		for (std::uint32_t index = 0; index < count; ++index)
		{
			std::uint32_t id;
			std::uint64_t generation;
			std::uint64_t length;
			reader >> id >> generation >> length;
			result.push_back(id);
			reader.skip(static_cast<std::size_t>(length));
		}
		EXPECT_EQ(reader.remaining(), 0u);
		return result;
	}
}

TEST(PublishableContractTest, EmptyRegistriesProduceNoBytes)
{
	Publisher source;
	Publisher replica(Publisher::Mode::Replicant);
	EXPECT_TRUE(source.encodeInitialization().empty());
	EXPECT_TRUE(source.encodeUpdate().empty());
	EXPECT_TRUE(source.encodeUpdate(Publisher::UpdateSelection::All).empty());
	replica.decode(spk::ByteStream{}.reader());
}

TEST(PublishableContractTest, ZeroSectionFrameIsAccepted)
{
	Publisher replica(Publisher::Mode::Replicant);
	spk::ByteStream::Writer writer;
	writer << std::uint8_t{0} << std::uint32_t{0};
	replica.decode(std::move(writer).build().reader());
}

TEST(PublishableContractTest, TruncatedPrefixesAreRejectedWithoutApplyingParts)
{
	Publisher replica(Publisher::Mode::Replicant);
	Probe part;
	replica.setPartRegistry({{0, part, {}}});
	const auto valid = message(1, 0, 4, 12);
	for (std::size_t length = 1; length < valid.size(); ++length)
	{
		spk::ByteStream::Writer writer;
		writer.append(valid.data().data(), length);
		EXPECT_THROW(replica.decode(std::move(writer).build().reader()), spk::Exception);
	}
	EXPECT_EQ(part.decoded, 0);
	replica.decode(valid.reader());
	EXPECT_EQ(part.value, 12);
}

TEST(PublishableContractTest, ExtraTrailingByteIsRejectedBeforeDecode)
{
	Publisher replica(Publisher::Mode::Replicant);
	Probe part;
	replica.setPartRegistry({{0, part, {}}});
	const auto valid = message(1, 0, 1, 20);
	spk::ByteStream::Writer writer;
	writer.append(valid.data().data(), valid.size());
	writer << std::uint8_t{99};
	EXPECT_THROW(replica.decode(std::move(writer).build().reader()), spk::Exception);
	EXPECT_EQ(part.decoded, 0);
}

TEST(PublishableContractTest, ForgedSectionLengthIsRejected)
{
	Publisher replica(Publisher::Mode::Replicant);
	Probe part;
	replica.setPartRegistry({{0, part, {}}});
	spk::ByteStream::Writer writer;
	writer << std::uint8_t{1} << std::uint32_t{1} << std::uint32_t{0} << std::uint64_t{0};
	writer << std::numeric_limits<std::uint64_t>::max();
	EXPECT_THROW(replica.decode(std::move(writer).build().reader()), spk::Exception);
	EXPECT_EQ(part.decoded, 0);
}

TEST(PublishableContractTest, ChangedRegistryDuringEncodeKeepsOperationSnapshot)
{
	Publisher source;
	Probe first;
	Probe second;
	source.setPartRegistry({{0, first, Publisher::Publication::Update}, {1, second, Publisher::Publication::Update}});
	first.duringEncode = [&]() {
		source.setPartRegistry({{7, second, Publisher::Publication::Update}});
	};
	EXPECT_EQ(ids(source.encodeInitialization()), (std::vector<std::uint32_t>{0, 1}));
	EXPECT_EQ(ids(source.encodeInitialization()), (std::vector<std::uint32_t>{7}));
	EXPECT_EQ(first.captured, 1);
	EXPECT_EQ(second.captured, 1);
}

TEST(PublishableContractTest, ChangedRegistryDuringDecodeKeepsOperationSnapshot)
{
	Publisher replica(Publisher::Mode::Replicant);
	Probe part;
	replica.setPartRegistry({{0, part, {}}});
	part.duringDecode = [&]() {
		replica.setPartRegistry({});
	};
	replica.decode(message(0, 0, 3, 55).reader());
	EXPECT_EQ(part.value, 55);
	EXPECT_TRUE(replica.encodeInitialization().empty());
}

TEST(PublishableContractTest, FailedDecoderCanRecoverSameGeneration)
{
	Publisher replica(Publisher::Mode::Replicant);
	Probe part;
	replica.setPartRegistry({{0, part, {}}});
	part.duringDecode = []() {
		throw spk::Exception("Temporary failure.");
	};
	const auto bytes = message(1, 0, 9, 44);
	EXPECT_THROW(replica.decode(bytes.reader()), spk::Exception);
	part.duringDecode = {};
	replica.decode(bytes.reader());
	EXPECT_EQ(part.value, 44);
	EXPECT_EQ(part.decoded, 1);
}

TEST(PublishableContractTest, FirstDecoderFailurePropagatesWithoutSkippingOthers)
{
	Publisher replica(Publisher::Mode::Replicant);
	Probe first;
	Probe second;
	replica.setPartRegistry({{0, first, {}}, {1, second, {}}});
	first.duringDecode = []() {
		throw spk::Exception("First failure.");
	};
	second.duringDecode = []() {
		throw spk::Exception("Second failure.");
	};
	spk::ByteStream::Writer writer;
	writer << std::uint8_t{0} << std::uint32_t{2};
	for (std::uint32_t id = 0; id < 2; ++id)
	{
		writer << id << std::uint64_t{1} << std::uint64_t{sizeof(int)} << static_cast<int>(id + 10);
	}
	try
	{
		replica.decode(std::move(writer).build().reader());
		FAIL() << "Expected the first decoder exception.";
	}
	catch (const spk::Exception &exception)
	{
		EXPECT_NE(std::string(exception.what()).find("First failure."), std::string::npos);
	}
}

TEST(PublishableContractTest, ReceiverTracksMaximumGenerationWithoutWraparound)
{
	Publisher replica(Publisher::Mode::Replicant);
	Probe part;
	replica.setPartRegistry({{0, part, {}}});
	const auto maximum = std::numeric_limits<std::uint64_t>::max();
	replica.decode(message(0, 0, maximum, 4).reader());
	replica.decode(message(1, 0, maximum, 9).reader());
	EXPECT_EQ(part.value, 4);
	EXPECT_EQ(part.decoded, 1);
}

TEST(PublishableContractTest, InvalidModeAndSelectionAreExplicitErrors)
{
	EXPECT_THROW((void)Publisher(static_cast<Publisher::Mode>(42)), spk::Exception);
	Publisher source;
	EXPECT_THROW((void)source.encodeUpdate(static_cast<Publisher::UpdateSelection>(44)), spk::Exception);
}

TEST(PublishableContractTest, PartsCanMoveBetweenPublishablesAfterUnregistration)
{
	Publisher first;
	Publisher second;
	Probe part;
	first.setPartRegistry({{4, part, {}}});
	EXPECT_THROW(second.setPartRegistry({{4, part, {}}}), spk::Exception);
	first.setPartRegistry({});
	second.setPartRegistry({{4, part, {}}});
	EXPECT_EQ(ids(second.encodeInitialization()), (std::vector<std::uint32_t>{4}));
}

TEST(PublishableContractTest, OwnerDestructionAllowsPartToBeReused)
{
	Probe part;
	{
		Publisher source;
		source.setPartRegistry({{0, part, {}}});
	}
	Publisher replacement;
	replacement.setPartRegistry({{3, part, {}}});
	EXPECT_EQ(ids(replacement.encodeInitialization()), (std::vector<std::uint32_t>{3}));
}
