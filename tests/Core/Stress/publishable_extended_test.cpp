#include "design_pattern/publishable.hpp"
#include "exception.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <random>
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

	class NumberPart final : public spk::Publishable::Part
	{
	public:
		std::int64_t value = 0;
		mutable unsigned captures = 0;
		unsigned applications = 0;

		void set(std::int64_t next)
		{
			value = next;
			invalidate();
		}

	protected:
		void _encode(spk::ByteStream::Writer &writer) const override
		{
			++captures;
			writer << value;
		}

		void _decode(const spk::ByteStream::Reader &reader) override
		{
			reader >> value;
			++applications;
		}
	};

	std::size_t countParts(const spk::ByteStream &encoded)
	{
		if (encoded.empty())
		{
			return 0;
		}
		const auto reader = encoded.reader();
		std::uint8_t mode;
		std::uint32_t count;
		reader >> mode >> count;
		return count;
	}
}

TEST(PublishableStressTest, RepeatedModifiedAndAllCyclesPreserveLatestData)
{
	Publisher source;
	Publisher replica(Publisher::Mode::Replicant);
	NumberPart local;
	NumberPart remote;
	source.setPartRegistry({{0, local, Publisher::Publication::Update}});
	replica.setPartRegistry({{0, remote, Publisher::Publication::Update}});
	std::mt19937_64 random(12345);
	replica.decode(source.encodeInitialization().reader());

	for (int i = 0; i < 3000; ++i)
	{
		const auto expected = static_cast<std::int64_t>(random());
		local.set(expected);
		const auto all = source.encodeUpdate(Publisher::UpdateSelection::All);
		const auto modified = source.encodeUpdate();
		EXPECT_EQ(countParts(all), 1u);
		EXPECT_EQ(countParts(modified), 1u);
		EXPECT_TRUE(source.encodeUpdate().empty());

		replica.decode(modified.reader());
		replica.decode(all.reader());
		EXPECT_EQ(remote.value, expected);
	}
	EXPECT_EQ(local.captures, 3001u);
	EXPECT_EQ(remote.applications, 3001u);
}

TEST(PublishableStressTest, WholeRegistryReplacementAndPartReuse)
{
	Publisher publisher;
	std::vector<std::unique_ptr<NumberPart>> parts;
	parts.reserve(128);
	for (int i = 0; i < 128; ++i)
	{
		parts.push_back(std::make_unique<NumberPart>());
	}

	for (int cycle = 0; cycle < 200; ++cycle)
	{
		Publisher::PartRegistry registry;
		for (int index = 0; index < 128; ++index)
		{
			parts[index]->set(cycle * 128 + index);
			registry.push_back({static_cast<std::uint32_t>(127 - index), *parts[index], Publisher::Publication::Update});
		}
		publisher.setPartRegistry(std::move(registry));
		EXPECT_EQ(countParts(publisher.encodeUpdate()), 128u);
		EXPECT_TRUE(publisher.encodeUpdate().empty());
	}
	publisher.setPartRegistry({});
	EXPECT_TRUE(publisher.encodeInitialization().empty());
}

TEST(PublishableMemorySafetyTest, DestroyedPartWithdrawsRegistrationAcrossIterations)
{
	Publisher publisher;
	for (int i = 0; i < 1500; ++i)
	{
		auto owned = std::make_unique<NumberPart>();
		publisher.setPartRegistry({{0, *owned, Publisher::Publication::Update}});
		EXPECT_EQ(countParts(publisher.encodeInitialization()), 1u);
		owned.reset();
		EXPECT_TRUE(publisher.encodeUpdate().empty());
	}
}

TEST(PublishableMemorySafetyTest, DestroyingPublishableReleasesPartOwnership)
{
	NumberPart shared;
	for (int i = 0; i < 1000; ++i)
	{
		Publisher publisher;
		publisher.setPartRegistry({{0, shared, Publisher::Publication::Update}});
		(void)publisher.encodeUpdate();
	}
}

TEST(PublishableStressTest, NewBindingsStartModifiedAfterRepeatedReplacement)
{
	Publisher publisher;
	NumberPart first;
	NumberPart second;
	for (int i = 0; i < 1000; ++i)
	{
		publisher.setPartRegistry({{0, first, Publisher::Publication::Update}});
		EXPECT_EQ(countParts(publisher.encodeUpdate()), 1u);
		publisher.setPartRegistry({{0, second, Publisher::Publication::Update}});
		EXPECT_EQ(countParts(publisher.encodeUpdate()), 1u);
	}
}

TEST(PublishableContractTest, UpdateCapturesCannotBeCommittedToAnotherPublishable)
{
	Publisher first;
	Publisher second;
	NumberPart a;
	NumberPart b;
	first.setPartRegistry({{0, a, Publisher::Publication::Update}});
	second.setPartRegistry({{0, b, Publisher::Publication::Update}});

	const auto capture = first.captureUpdate();
	EXPECT_THROW(second.commitUpdate(capture), spk::Exception);
	EXPECT_EQ(countParts(first.encodeUpdate()), 1u);
	EXPECT_EQ(countParts(second.encodeUpdate()), 1u);
}

TEST(PublishableContractTest, EmptyCaptureIsRejectedByPublicAccessors)
{
	Publisher publisher;
	Publisher::UpdateCapture empty;
	EXPECT_THROW((void)empty.bytes(), spk::Exception);
	EXPECT_THROW(publisher.commitUpdate(empty), spk::Exception);
}
