#include "design_pattern/publishable.hpp"
#include "exception.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
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

	class ValuePart : public spk::Publishable::Part
	{
	public:
		int value = 0;
		mutable int encodes = 0;
		int decodes = 0;
		bool failEncode = false;
		bool failDecode = false;
		bool leaveUnread = false;

		void change(int next)
		{
			value = next;
			invalidate();
		}

	protected:
		void _encode(spk::ByteStream::Writer &writer) const override
		{
			++encodes;
			if (failEncode)
			{
				throw spk::Exception("Test encoder failure.");
			}
			writer << value;
		}

		void _decode(const spk::ByteStream::Reader &reader) override
		{
			if (failDecode)
			{
				throw spk::Exception("Test decoder failure.");
			}
			if (!leaveUnread)
			{
				reader >> value;
			}
			++decodes;
		}
	};

	struct Section
	{
		std::uint32_t id;
		std::uint64_t generation;
		int value;
	};

	std::vector<Section> unpack(const spk::ByteStream &stream)
	{
		if (stream.empty())
		{
			return {};
		}
		const auto reader = stream.reader();
		std::uint8_t mode;
		std::uint32_t count;
		reader >> mode >> count;
		std::vector<Section> result;
		for (std::uint32_t i = 0; i < count; ++i)
		{
			std::uint32_t id;
			std::uint64_t generation;
			std::uint64_t length;
			reader >> id >> generation >> length;
			if (length != sizeof(int))
			{
				throw std::runtime_error("Unexpected test payload.");
			}
			int value;
			reader >> value;
			result.push_back({id, generation, value});
		}
		EXPECT_EQ(reader.remaining(), 0u);
		return result;
	}

	spk::ByteStream frame(std::uint8_t mode, const std::vector<Section> &sections)
	{
		spk::ByteStream::Writer writer;
		writer << mode << static_cast<std::uint32_t>(sections.size());
		for (const Section &section : sections)
		{
			writer << section.id << section.generation << std::uint64_t{sizeof(int)} << section.value;
		}
		return std::move(writer).build();
	}
}

TEST(PublishableTest, InitializationContainsAllAndModifiedConsumesOnlyUpdates)
{
	Publisher publisher;
	ValuePart changing;
	ValuePart initializationOnly;
	changing.value = 7;
	initializationOnly.value = 12;
	publisher.setPartRegistry({{5, changing, Publisher::Publication::Update}, {2, initializationOnly, {}}});

	const auto initial = unpack(publisher.encodeInitialization());
	ASSERT_EQ(initial.size(), 2u);
	EXPECT_EQ(initial[0].id, 2u);
	EXPECT_EQ(initial[1].id, 5u);
	EXPECT_EQ(initial[1].generation, 0u);
	EXPECT_EQ(changing.encodes, 1);
	EXPECT_EQ(initializationOnly.encodes, 1);

	const auto modified = unpack(publisher.encodeUpdate());
	ASSERT_EQ(modified.size(), 1u);
	EXPECT_EQ(modified[0].value, 7);
	EXPECT_TRUE(publisher.encodeUpdate().empty());
	EXPECT_EQ(changing.encodes, 1);

	initializationOnly.change(19);
	EXPECT_TRUE(publisher.encodeUpdate().empty());
	EXPECT_EQ(unpack(publisher.encodeInitialization())[0].generation, 1u);
	EXPECT_EQ(initializationOnly.encodes, 2);
}

TEST(PublishableTest, AllDoesNotConsumePendingChanges)
{
	Publisher publisher;
	ValuePart value;
	publisher.setPartRegistry({{0, value, Publisher::Publication::Update}});

	ASSERT_EQ(unpack(publisher.encodeUpdate(Publisher::UpdateSelection::All)).size(), 1u);
	EXPECT_EQ(unpack(publisher.encodeUpdate())[0].generation, 0u);
	EXPECT_TRUE(publisher.encodeUpdate().empty());

	value.change(50);
	ASSERT_EQ(unpack(publisher.encodeUpdate(Publisher::UpdateSelection::All))[0].value, 50);
	EXPECT_EQ(unpack(publisher.encodeUpdate())[0].generation, 1u);
	EXPECT_TRUE(publisher.encodeUpdate().empty());
}

TEST(PublishableTest, SnapshotIsImmutableAndLazilyReused)
{
	Publisher publisher;
	ValuePart value;
	value.value = 11;
	publisher.setPartRegistry({{0, value, Publisher::Publication::Update}});

	const auto saved = publisher.encodeInitialization();
	(void)publisher.encodeInitialization();
	(void)publisher.encodeUpdate(Publisher::UpdateSelection::All);
	EXPECT_EQ(value.encodes, 1);
	value.change(22);
	EXPECT_EQ(unpack(saved)[0].value, 11);
	EXPECT_EQ(unpack(publisher.encodeInitialization())[0].value, 22);
	EXPECT_EQ(value.encodes, 2);
}

TEST(PublishableTest, ChangedBindingAndFlagsResetModifiedState)
{
	Publisher publisher;
	ValuePart value;
	publisher.setPartRegistry({{1, value, Publisher::Publication::Update}});
	(void)publisher.encodeUpdate();
	EXPECT_TRUE(publisher.encodeUpdate().empty());

	publisher.setPartRegistry({{1, value, Publisher::Publication::Update}});
	EXPECT_TRUE(publisher.encodeUpdate().empty());
	publisher.setPartRegistry({{1, value, {}}});
	EXPECT_TRUE(publisher.encodeUpdate().empty());
	publisher.setPartRegistry({{1, value, Publisher::Publication::Update}});
	EXPECT_EQ(unpack(publisher.encodeUpdate())[0].id, 1u);

	publisher.setPartRegistry({{42, value, Publisher::Publication::Update}});
	EXPECT_EQ(unpack(publisher.encodeUpdate())[0].id, 42u);
	EXPECT_EQ(value.encodes, 1);
}

TEST(PublishableTest, ArbitraryPartIDsCanBeReassigned)
{
	Publisher publisher;
	ValuePart first;
	ValuePart second;
	first.value = 3;
	second.value = 9;
	publisher.setPartRegistry({{0, first, Publisher::Publication::Update}});
	(void)publisher.encodeUpdate();

	publisher.setPartRegistry({{0, second, Publisher::Publication::Update}});
	ASSERT_EQ(unpack(publisher.encodeUpdate())[0].value, 9);
	publisher.setPartRegistry({{0, first, Publisher::Publication::Update}});
	EXPECT_EQ(unpack(publisher.encodeUpdate())[0].value, 3);
}

TEST(PublishableTest, SourceGenerationAdvancesAndReplicantInvalidationIsNoOp)
{
	Publisher source(Publisher::Mode::Source);
	Publisher replica(Publisher::Mode::Replicant);
	ValuePart sourceValue;
	ValuePart replicaValue;
	sourceValue.change(7);
	source.setPartRegistry({{0, sourceValue, Publisher::Publication::Update}});
	replica.setPartRegistry({{0, replicaValue, Publisher::Publication::Update}});

	replica.decode(source.encodeInitialization().reader());
	EXPECT_EQ(replicaValue.value, 7);
	EXPECT_EQ(replicaValue.decodes, 1);
	replicaValue.change(88);
	EXPECT_EQ(unpack(replica.encodeInitialization())[0].generation, 0u);
	replicaValue.change(99);
	EXPECT_EQ(unpack(replica.encodeInitialization())[0].value, 88);
	EXPECT_EQ(unpack(source.encodeInitialization())[0].generation, 1u);
}

TEST(PublishableTest, DuplicateAndForeignRegistrationsRollBack)
{
	Publisher publisher;
	Publisher other;
	ValuePart first;
	ValuePart second;
	publisher.setPartRegistry({{0, first, Publisher::Publication::Update}});
	EXPECT_THROW(publisher.setPartRegistry({{1, first, {}}, {1, second, {}}}), spk::Exception);
	EXPECT_THROW(publisher.setPartRegistry({{1, first, {}}, {2, first, {}}}), spk::Exception);
	EXPECT_THROW(publisher.setPartRegistry({{4, second, static_cast<Publisher::Publication>(8)}}), spk::Exception);
	EXPECT_THROW(other.setPartRegistry({{0, first, {}}}), spk::Exception);
	EXPECT_EQ(unpack(publisher.encodeUpdate())[0].id, 0u);
	other.setPartRegistry({{0, second, {}}});
	EXPECT_EQ(unpack(other.encodeInitialization())[0].id, 0u);
}

TEST(PublishableTest, FailedEncodeLeavesModifiedPending)
{
	Publisher publisher;
	ValuePart first;
	ValuePart second;
	publisher.setPartRegistry({
		{0, first, Publisher::Publication::Update},
		{1, second, Publisher::Publication::Update}});
	second.failEncode = true;
	EXPECT_THROW((void)publisher.encodeUpdate(), spk::Exception);
	second.failEncode = false;
	EXPECT_EQ(unpack(publisher.encodeUpdate()).size(), 2u);
	EXPECT_TRUE(publisher.encodeUpdate().empty());
}

TEST(PublishableTest, SuccessfulPartsCommitDespiteOtherDecoderFailure)
{
	Publisher replica(Publisher::Mode::Replicant);
	ValuePart first;
	ValuePart second;
	replica.setPartRegistry({{0, first, Publisher::Publication::Update}, {1, second, Publisher::Publication::Update}});
	first.failDecode = true;
	const auto message = frame(1, {{0, 5, 17}, {1, 3, 33}});
	EXPECT_THROW(replica.decode(message.reader()), spk::Exception);
	EXPECT_EQ(first.decodes, 0);
	EXPECT_EQ(second.value, 33);
	EXPECT_EQ(second.decodes, 1);

	first.failDecode = false;
	replica.decode(message.reader());
	EXPECT_EQ(first.value, 17);
	EXPECT_EQ(first.decodes, 1);
	EXPECT_EQ(second.decodes, 1);
}

TEST(PublishableTest, ZeroGenerationAcceptedOnceAndOlderIgnored)
{
	Publisher replica(Publisher::Mode::Replicant);
	ValuePart value;
	replica.setPartRegistry({{0, value, Publisher::Publication::Update}});
	replica.decode(frame(0, {{0, 0, 4}}).reader());
	replica.decode(frame(1, {{0, 0, 5}}).reader());
	EXPECT_EQ(value.value, 4);
	EXPECT_EQ(value.decodes, 1);
	replica.decode(frame(1, {{0, 1, 6}}).reader());
	EXPECT_EQ(value.value, 6);
}

TEST(PublishableTest, ChangingBindingResetsReceivedGeneration)
{
	Publisher replica(Publisher::Mode::Replicant);
	ValuePart first;
	ValuePart second;
	replica.setPartRegistry({{0, first, {}}});
	replica.decode(frame(0, {{0, 90, 10}}).reader());
	replica.setPartRegistry({{0, second, {}}});
	replica.decode(frame(0, {{0, 0, 20}}).reader());
	EXPECT_EQ(second.value, 20);
	replica.setPartRegistry({{0, first, {}}});
	replica.decode(frame(0, {{0, 0, 30}}).reader());
	EXPECT_EQ(first.value, 30);
}

TEST(PublishableTest, InvalidFrameIsRejectedBeforeAnyPartIsApplied)
{
	Publisher replica(Publisher::Mode::Replicant);
	ValuePart part;
	replica.setPartRegistry({{0, part, {}}});

	EXPECT_THROW(replica.decode(frame(2, {{0, 1, 4}}).reader()), spk::Exception);
	EXPECT_THROW(replica.decode(frame(1, {{0, 1, 4}, {0, 2, 5}}).reader()), spk::Exception);
	EXPECT_THROW(replica.decode(frame(1, {{0, 1, 4}, {9, 2, 5}}).reader()), spk::Exception);
	EXPECT_EQ(part.decodes, 0);
}

TEST(PublishableTest, IncompleteDecoderDoesNotAdvanceGeneration)
{
	Publisher replica(Publisher::Mode::Replicant);
	ValuePart part;
	replica.setPartRegistry({{0, part, {}}});
	part.leaveUnread = true;
	const auto message = frame(1, {{0, 7, 8}});
	EXPECT_THROW(replica.decode(message.reader()), spk::Exception);
	part.leaveUnread = false;
	replica.decode(message.reader());
	EXPECT_EQ(part.value, 8);
}

TEST(PublishableTest, PartDestructionWithdrawsItsRegistration)
{
	Publisher publisher;
	auto part = std::make_unique<ValuePart>();
	publisher.setPartRegistry({{0, *part, Publisher::Publication::Update}});
	part.reset();
	EXPECT_TRUE(publisher.encodeInitialization().empty());
	EXPECT_TRUE(publisher.encodeUpdate().empty());
}
