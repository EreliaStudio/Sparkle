#include "design_pattern/publishable.hpp"
#include "exception.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <numeric>
#include <random>
#include <thread>
#include <utility>
#include <vector>
#include <latch>

namespace
{
	class Publisher final : public spk::Publishable
	{
	public:
		using Publishable::add;
		using Publishable::contains;
		using Publishable::remove;
	};

	class CountingPart final : public spk::Publishable::Part
	{
	public:
		int value = 0;
		mutable int fullCaptures = 0;
		mutable int updateCaptures = 0;
		int fullDecodes = 0;
		int updateDecodes = 0;
		bool failDecode = false;
		bool failUpdate = false;

		void change(int next)
		{
			value = next;
			invalidate();
		}

	protected:
		void _encode(spk::ByteStream::Writer &writer) const override
		{
			++fullCaptures;
			writer << value;
		}

		void _decode(const spk::ByteStream::Reader &reader) override
		{
			if (failDecode)
			{
				throw spk::Exception("Test full decoder rejected the value.");
			}
			reader >> value;
			++fullDecodes;
		}

		void _encodeUpdate(spk::ByteStream::Writer &writer) const override
		{
			++updateCaptures;
			writer << value;
		}

		void _decodeUpdate(const spk::ByteStream::Reader &reader) override
		{
			if (failUpdate)
			{
				throw spk::Exception("Test update decoder rejected the value.");
			}
			reader >> value;
			++updateDecodes;
		}
	};

	class AtomicPart final : public spk::Publishable::Part
	{
	public:
		std::atomic<int> value{0};

		void change(int next)
		{
			value.store(next);
			invalidate();
		}

	protected:
		void _encode(spk::ByteStream::Writer &writer) const override
		{
			writer << value.load();
		}

		void _decode(const spk::ByteStream::Reader &reader) override
		{
			int next;
			reader >> next;
			value.store(next);
		}
	};

	spk::ByteStream integerPayload(int value)
	{
		spk::ByteStream::Writer writer;
		writer << value;
		return std::move(writer).build();
	}

	spk::ByteStream frame(
		std::uint8_t mode,
		const std::vector<std::pair<std::uint32_t, spk::ByteStream>> &parts)
	{
		spk::ByteStream::Writer writer;
		writer << mode << static_cast<std::uint32_t>(parts.size());
		for (const auto &[id, payload] : parts)
		{
			writer << id << static_cast<std::uint64_t>(payload.size());
			writer.append(payload.data().data(), payload.size());
		}
		return std::move(writer).build();
	}

	spk::ByteStream prefix(const spk::ByteStream &bytes, std::size_t length)
	{
		spk::ByteStream::Writer writer;
		writer.append(bytes.data().data(), length);
		return std::move(writer).build();
	}

	std::vector<std::pair<std::uint32_t, int>> unpackIntegers(const spk::ByteStream &bytes, std::uint8_t expectedMode)
	{
		auto reader = bytes.reader();
		EXPECT_EQ(reader.get<std::uint8_t>(), expectedMode);
		const std::uint32_t count = reader.get<std::uint32_t>();
		std::vector<std::pair<std::uint32_t, int>> values;
		for (std::uint32_t index = 0; index < count; ++index)
		{
			const std::uint32_t id = reader.get<std::uint32_t>();
			EXPECT_EQ(reader.get<std::uint64_t>(), sizeof(int));
			values.emplace_back(id, reader.get<int>());
		}
		EXPECT_EQ(reader.remaining(), 0u);
		return values;
	}
}

TEST(PublishableTest, PublishedSnapshotRemainsImmutableAfterFurtherChanges)
{
	Publisher publisher;
	CountingPart part;
	publisher.add(3, &part, Publisher::Publication::Edition);
	part.change(41);

	const spk::ByteStream earlier = publisher.encodeEdited();
	const spk::ByteStream copy = earlier;
	part.change(99);
	const spk::ByteStream later = publisher.encodeEdited();

	EXPECT_EQ(unpackIntegers(earlier, 1), (std::vector<std::pair<std::uint32_t, int>>{{3, 41}}));
	EXPECT_EQ(unpackIntegers(copy, 1), (std::vector<std::pair<std::uint32_t, int>>{{3, 41}}));
	EXPECT_EQ(unpackIntegers(later, 1), (std::vector<std::pair<std::uint32_t, int>>{{3, 99}}));
}

TEST(PublishableTest, SnapshotSurvivesPartDestruction)
{
	Publisher publisher;
	spk::ByteStream snapshot;
	{
		CountingPart part;
		publisher.add(3, &part, Publisher::Publication::Edition);
		part.change(7);
		snapshot = publisher.encodeEdited();
	}
	EXPECT_FALSE(publisher.contains(3));
	EXPECT_EQ(unpackIntegers(snapshot, 1), (std::vector<std::pair<std::uint32_t, int>>{{3, 7}}));
}

TEST(PublishableTest, RegistrationDoesNotEnqueuePreviouslyInvalidatedPart)
{
	CountingPart part;
	part.change(10);
	Publisher publisher;
	publisher.add(4, &part, Publisher::Publication::Edition);
	EXPECT_TRUE(publisher.encodeEdited().empty());
	EXPECT_EQ(unpackIntegers(publisher.encodeInitialization(), 0), (std::vector<std::pair<std::uint32_t, int>>{{4, 10}}));
	part.change(20);
	EXPECT_EQ(unpackIntegers(publisher.encodeEdited(), 1), (std::vector<std::pair<std::uint32_t, int>>{{4, 20}}));
}

TEST(PublishableTest, ReaddingSamePartWithDifferentIDWorks)
{
	Publisher publisher;
	CountingPart part;
	publisher.add(1, &part, Publisher::Publication::Edition);
	publisher.remove(1);
	publisher.add(2, &part, Publisher::Publication::Edition);
	part.change(17);
	EXPECT_FALSE(publisher.contains(1));
	EXPECT_TRUE(publisher.contains(2));
	EXPECT_EQ(unpackIntegers(publisher.encodeEdited(), 1), (std::vector<std::pair<std::uint32_t, int>>{{2, 17}}));
}

TEST(PublishableTest, FailedDuplicateRegistrationDoesNotRemoveOriginalSubscription)
{
	Publisher publisher;
	CountingPart part;
	publisher.add(1, &part, Publisher::Publication::Edition);
	EXPECT_THROW(publisher.add(2, &part, Publisher::Publication::Edition), spk::Exception);
	EXPECT_THROW(publisher.add(1, &part, Publisher::Publication::Edition), spk::Exception);
	part.change(23);
	EXPECT_EQ(unpackIntegers(publisher.encodeEdited(), 1), (std::vector<std::pair<std::uint32_t, int>>{{1, 23}}));
	EXPECT_TRUE(publisher.contains(1));
	EXPECT_FALSE(publisher.contains(2));
}

TEST(PublishableTest, SamePartSharesOneLazySnapshotAcrossPublishables)
{
	Publisher first;
	Publisher second;
	CountingPart part;
	first.add(1, &part, Publisher::Publication::Edition | Publisher::Publication::Update);
	second.add(2, &part, Publisher::Publication::Edition | Publisher::Publication::Update);
	part.change(3);

	EXPECT_FALSE(first.encodeInitialization().empty());
	EXPECT_FALSE(second.encodeInitialization().empty());
	EXPECT_EQ(part.fullCaptures, 1);
	EXPECT_FALSE(first.encodeUpdate().empty());
	EXPECT_FALSE(second.encodeUpdate().empty());
	EXPECT_EQ(part.updateCaptures, 1);

	part.change(4);
	EXPECT_FALSE(first.encodeEdited().empty());
	EXPECT_FALSE(second.encodeEdited().empty());
	EXPECT_EQ(part.fullCaptures, 2);
	EXPECT_FALSE(first.encodeUpdate().empty());
	EXPECT_EQ(part.updateCaptures, 2);
}

TEST(PublishableTest, EditedAndUpdateCachesRemainIndependent)
{
	Publisher publisher;
	CountingPart part;
	publisher.add(1, &part, Publisher::Publication::Edition | Publisher::Publication::Update);

	part.change(1);
	EXPECT_FALSE(publisher.encodeEdited().empty());
	EXPECT_EQ(part.fullCaptures, 1);
	EXPECT_EQ(part.updateCaptures, 0);
	EXPECT_FALSE(publisher.encodeUpdate().empty());
	EXPECT_EQ(part.updateCaptures, 1);
	EXPECT_FALSE(publisher.encodeUpdate().empty());
	EXPECT_EQ(part.updateCaptures, 1);
	EXPECT_TRUE(publisher.encodeEdited().empty());

	part.change(2);
	EXPECT_FALSE(publisher.encodeUpdate().empty());
	EXPECT_EQ(part.updateCaptures, 2);
	EXPECT_EQ(part.fullCaptures, 1);
	EXPECT_FALSE(publisher.encodeEdited().empty());
	EXPECT_EQ(part.fullCaptures, 2);
}

TEST(PublishableTest, IDsAtBothUint32BoundariesAreEncodedWithoutRemapping)
{
	Publisher publisher;
	CountingPart highest;
	CountingPart lowest;
	highest.value = 70;
	lowest.value = 10;
	constexpr std::uint32_t lastID = std::numeric_limits<std::uint32_t>::max();
	publisher.add(lastID, &highest, Publisher::Publication::Edition | Publisher::Publication::Update);
	publisher.add(0, &lowest, Publisher::Publication::Edition | Publisher::Publication::Update);

	const std::vector<std::pair<std::uint32_t, int>> expected{{0, 10}, {lastID, 70}};
	EXPECT_EQ(unpackIntegers(publisher.encodeInitialization(), 0), expected);
	EXPECT_EQ(unpackIntegers(publisher.encodeUpdate(), 2), expected);
	highest.change(80);
	lowest.change(20);
	EXPECT_EQ(unpackIntegers(publisher.encodeEdited(), 1),
		(std::vector<std::pair<std::uint32_t, int>>{{0, 20}, {lastID, 80}}));
}

TEST(PublishableTest, IndependentRegistrationOrdersProduceSameWireBytes)
{
	Publisher first;
	Publisher second;
	std::array<CountingPart, 3> left;
	std::array<CountingPart, 3> right;
	for (int index = 0; index < 3; ++index)
	{
		left[index].value = index + 50;
		right[index].value = index + 50;
	}
	first.add(2, &left[2]);
	first.add(0, &left[0]);
	first.add(1, &left[1]);
	second.add(1, &right[1]);
	second.add(2, &right[2]);
	second.add(0, &right[0]);

	const auto a = first.encodeInitialization();
	const auto b = second.encodeInitialization();
	EXPECT_EQ(a.data().size(), b.data().size());
	EXPECT_TRUE(std::equal(a.data().begin(), a.data().end(), b.data().begin(), b.data().end()));
}

TEST(PublishableTest, AllTruncatedPrefixesOfValidPublicationThrow)
{
	Publisher publisher;
	CountingPart part;
	publisher.add(1, &part);
	part.value = 14;
	const spk::ByteStream complete = publisher.encodeInitialization();
	ASSERT_GT(complete.size(), 1u);
	for (std::size_t length = 1; length < complete.size(); ++length)
	{
		SCOPED_TRACE(length);
		EXPECT_THROW(publisher.decode(prefix(complete, length).reader()), spk::Exception);
		EXPECT_EQ(part.fullDecodes, 0);
	}
	EXPECT_NO_THROW(publisher.decode(complete.reader()));
	EXPECT_EQ(part.fullDecodes, 1);
}

TEST(PublishableTest, AllTruncatedPrefixesWithTwoPartsFailBeforeAnyDecoder)
{
	Publisher publisher;
	CountingPart first;
	CountingPart second;
	publisher.add(1, &first);
	publisher.add(2, &second);
	const spk::ByteStream complete = frame(1, {{1, integerPayload(10)}, {2, integerPayload(20)}});
	for (std::size_t length = 1; length < complete.size(); ++length)
	{
		SCOPED_TRACE(length);
		EXPECT_THROW(publisher.decode(prefix(complete, length).reader()), spk::Exception);
		EXPECT_EQ(first.fullDecodes, 0);
		EXPECT_EQ(second.fullDecodes, 0);
	}
}

TEST(PublishableTest, UnknownOnlyCollectionWithSkipDoesNotApplyData)
{
	Publisher publisher;
	CountingPart part;
	part.value = 77;
	publisher.add(1, &part);
	const auto unknown = frame(1, {{333, integerPayload(12)}, {444, integerPayload(13)}});
	EXPECT_NO_THROW(publisher.decode(unknown.reader(), Publisher::UnknownPartBehaviour::Skip));
	EXPECT_EQ(part.value, 77);
	EXPECT_EQ(part.fullDecodes, 0);
}

TEST(PublishableTest, SkipPolicyStillRejectsUnknownDuplicates)
{
	Publisher publisher;
	const auto duplicateUnknown = frame(1, {{333, integerPayload(2)}, {333, integerPayload(3)}});
	EXPECT_THROW(publisher.decode(duplicateUnknown.reader(), Publisher::UnknownPartBehaviour::Skip), spk::Exception);
}

TEST(PublishableTest, SkipPolicyDoesNotIgnoreMalformedUnknownPayload)
{
	Publisher publisher;
	CountingPart known;
	known.value = 17;
	publisher.add(1, &known);

	spk::ByteStream::Writer writer;
	writer << std::uint8_t{2} << std::uint32_t{2};
	writer << std::uint32_t{1} << std::uint64_t{sizeof(int)} << int{21};
	writer << std::uint32_t{444} << std::uint64_t{1024} << int{22};
	const auto malformed = std::move(writer).build();

	EXPECT_THROW(publisher.decode(malformed.reader(), Publisher::UnknownPartBehaviour::Skip), spk::Exception);
	EXPECT_EQ(known.value, 17);
	EXPECT_EQ(known.updateDecodes, 0);
}

TEST(PublishableTest, CountTooLargeForReaderIsRejectedBeforeAllocation)
{
	Publisher publisher;
	spk::ByteStream::Writer writer;
	writer << std::uint8_t{0} << std::numeric_limits<std::uint32_t>::max();
	EXPECT_THROW(publisher.decode(std::move(writer).build().reader()), spk::Exception);
}

TEST(PublishableTest, MaxUint64PayloadLengthIsRejected)
{
	Publisher publisher;
	spk::ByteStream::Writer writer;
	writer << std::uint8_t{0} << std::uint32_t{1};
	writer << std::uint32_t{111} << std::numeric_limits<std::uint64_t>::max();
	EXPECT_THROW(publisher.decode(std::move(writer).build().reader(), Publisher::UnknownPartBehaviour::Skip), spk::Exception);
}

TEST(PublishableTest, ReaderRelativeOffsetsAreValidForNestedCollections)
{
	Publisher publisher;
	CountingPart part;
	publisher.add(5, &part);
	const spk::ByteStream payload = frame(1, {{5, integerPayload(77)}});
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{0xABCD1234};
	writer.append(payload.data().data(), payload.size());
	writer << std::uint32_t{0x1234ABCD};
	const auto outer = std::move(writer).build();

	auto reader = outer.reader(4, 4 + payload.size());
	EXPECT_NO_THROW(publisher.decode(reader));
	EXPECT_EQ(reader.remaining(), 0u);
	EXPECT_EQ(part.value, 77);
	EXPECT_EQ(part.fullDecodes, 1);
}

TEST(PublishableTest, DecoderFailureKeepsPriorSuccessfullyDecodedState)
{
	Publisher publisher;
	CountingPart first;
	CountingPart second;
	first.value = 3;
	second.value = 4;
	second.failDecode = true;
	publisher.add(1, &first);
	publisher.add(2, &second);
	const auto valid = frame(0, {{1, integerPayload(30)}, {2, integerPayload(40)}});
	EXPECT_THROW(publisher.decode(valid.reader()), spk::Exception);
	EXPECT_EQ(first.value, 30);
	EXPECT_EQ(first.fullDecodes, 1);
	EXPECT_EQ(second.value, 4);
	EXPECT_EQ(second.fullDecodes, 0);
}

TEST(PublishableTest, UpdateDecoderFailurePropagatesAndCanBeRetried)
{
	Publisher publisher;
	CountingPart part;
	part.failUpdate = true;
	publisher.add(1, &part);
	const auto incoming = frame(2, {{1, integerPayload(123)}});
	EXPECT_THROW(publisher.decode(incoming.reader()), spk::Exception);
	EXPECT_EQ(part.updateDecodes, 0);
	part.failUpdate = false;
	publisher.decode(incoming.reader());
	EXPECT_EQ(part.value, 123);
	EXPECT_EQ(part.updateDecodes, 1);
}

TEST(PublishableTest, RepeatedInitializationDoesNotConsumeEditedEvents)
{
	Publisher publisher;
	CountingPart part;
	publisher.add(1, &part, Publisher::Publication::Edition);
	part.change(123);
	const auto first = publisher.encodeInitialization();
	const auto second = publisher.encodeInitialization();
	EXPECT_EQ(first.data().size(), second.data().size());
	EXPECT_EQ(part.fullCaptures, 1);
	EXPECT_EQ(unpackIntegers(publisher.encodeEdited(), 1),
		(std::vector<std::pair<std::uint32_t, int>>{{1, 123}}));
	EXPECT_TRUE(publisher.encodeEdited().empty());
}

TEST(PublishableTest, UpdateOnlyPartIsNeverPublishedAsEdited)
{
	Publisher publisher;
	CountingPart part;
	publisher.add(9, &part, Publisher::Publication::Update);
	part.change(123);
	EXPECT_TRUE(publisher.encodeEdited().empty());
	EXPECT_EQ(unpackIntegers(publisher.encodeUpdate(), 2),
		(std::vector<std::pair<std::uint32_t, int>>{{9, 123}}));
	EXPECT_EQ(unpackIntegers(publisher.encodeInitialization(), 0),
		(std::vector<std::pair<std::uint32_t, int>>{{9, 123}}));
}

TEST(PublishableTest, InvalidatingOnePartDoesNotRebuildOtherPartCache)
{
	Publisher publisher;
	CountingPart changed;
	CountingPart untouched;
	publisher.add(1, &changed, Publisher::Publication::Edition | Publisher::Publication::Update);
	publisher.add(2, &untouched, Publisher::Publication::Edition | Publisher::Publication::Update);
	(void)publisher.encodeInitialization();
	(void)publisher.encodeUpdate();
	EXPECT_EQ(changed.fullCaptures, 1);
	EXPECT_EQ(untouched.fullCaptures, 1);

	changed.change(11);
	(void)publisher.encodeEdited();
	(void)publisher.encodeUpdate();
	EXPECT_EQ(changed.fullCaptures, 2);
	EXPECT_EQ(changed.updateCaptures, 2);
	EXPECT_EQ(untouched.fullCaptures, 1);
	EXPECT_EQ(untouched.updateCaptures, 1);
}

TEST(PublishableStressTest, ConcurrentRegistrationAndInvalidationCanRecoverFinalState)
{
	Publisher publisher;
	AtomicPart part;
	std::latch start(1);
	std::thread registration([&]() {
		start.wait();
		for (int i = 0; i < 1000; ++i)
		{
			publisher.add(4, &part, Publisher::Publication::Edition);
			publisher.remove(4);
		}
		publisher.add(4, &part, Publisher::Publication::Edition);
	});
	std::thread invalidations([&]() {
		start.wait();
		for (int i = 0; i < 1000; ++i)
		{
			part.change(i);
		}
	});
	start.count_down();
	registration.join();
	invalidations.join();
	ASSERT_TRUE(publisher.contains(4));
	part.change(1001);
	const auto latest = publisher.encodeEdited();
	ASSERT_FALSE(latest.empty());
	EXPECT_EQ(unpackIntegers(latest, 1),
		(std::vector<std::pair<std::uint32_t, int>>{{4, 1001}}));
}

TEST(PublishableStressTest, RepeatedMalformedInputCannotMutateValidParts)
{
	Publisher publisher;
	CountingPart part;
	part.value = 54;
	publisher.add(3, &part);
	std::mt19937 random(0xBADC0DE);
	std::uniform_int_distribution<int> lengthDist(1, 120);
	std::uniform_int_distribution<int> byteDist(0, 255);
	for (int i = 0; i < 1200; ++i)
	{
		const int length = lengthDist(random);
		std::vector<std::byte> bytes(static_cast<std::size_t>(length));
		for (std::byte &byte : bytes)
		{
			byte = static_cast<std::byte>(byteDist(random));
		}
		spk::ByteStream::Writer writer;
		writer.append(bytes.data(), bytes.size());
		const auto message = std::move(writer).build();
		try
		{
			publisher.decode(message.reader(), Publisher::UnknownPartBehaviour::Skip);
		} catch (const spk::Exception &)
		{
		}
		EXPECT_EQ(part.value, 54);
		EXPECT_EQ(part.fullDecodes, 0);
	}
}
