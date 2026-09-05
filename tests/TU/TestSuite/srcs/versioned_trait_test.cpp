#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "design_pattern/trait/versioned_trait.hpp"

namespace
{
	using spk::VersionedTrait;

	TEST(VersionedTraitTest, StandardUsageInvalidatesMonotonicallyAndNotifiesSubscribers)
	{
		VersionedTrait value;
		std::vector<VersionedTrait::Version> observedVersions;

		auto contract = value.subscribeToVersionEdition(
			[&](VersionedTrait *edited)
			{
				ASSERT_EQ(edited, &value);
				observedVersions.push_back(edited->version());
			});

		EXPECT_EQ(value.version(), 0u);

		value.invalidate();
		const auto firstVersion = value.version();
		value.invalidate();
		const auto secondVersion = value.version();
		value.invalidate();
		const auto thirdVersion = value.version();

		EXPECT_GT(firstVersion, 0u);
		EXPECT_GT(secondVersion, firstVersion);
		EXPECT_GT(thirdVersion, secondVersion);
		ASSERT_EQ(observedVersions.size(), 3u);
		EXPECT_EQ(observedVersions[0], firstVersion);
		EXPECT_EQ(observedVersions[1], secondVersion);
		EXPECT_EQ(observedVersions[2], thirdVersion);
	}

	TEST(VersionedTraitTest, SubscriberLifetimeEndsWhenContractIsDestroyed)
	{
		VersionedTrait value;
		std::size_t callbackCount = 0;

		{
			auto contract = value.subscribeToVersionEdition(
				[&](VersionedTrait *)
				{
					++callbackCount;
				});

			value.invalidate();
			EXPECT_EQ(callbackCount, 1u);
		}

		value.invalidate();
		EXPECT_EQ(callbackCount, 1u);
	}

	TEST(VersionedTraitTest, ExplicitContractResignationStopsNotifications)
	{
		VersionedTrait value;
		std::size_t callbackCount = 0;
		auto contract = value.subscribeToVersionEdition(
			[&](VersionedTrait *)
			{
				++callbackCount;
			});

		value.invalidate();
		ASSERT_EQ(callbackCount, 1u);

		contract.resign();
		value.invalidate();

		EXPECT_EQ(callbackCount, 1u);
	}

	TEST(VersionedTraitTest, MoveConstructionPreservesTheCurrentVersion)
	{
		VersionedTrait source;
		source.invalidate();
		source.invalidate();
		const auto versionBeforeMove = source.version();

		VersionedTrait destination(std::move(source));

		EXPECT_EQ(destination.version(), versionBeforeMove);
		destination.invalidate();
		EXPECT_GT(destination.version(), versionBeforeMove);
	}

	TEST(VersionedTraitTest, MovedFromObjectRemainsDestructibleAndIndependentlyUsable)
	{
		VersionedTrait source;
		source.invalidate();

		VersionedTrait destination(std::move(source));
		const auto destinationVersion = destination.version();
		const auto sourceVersion = source.version();

		EXPECT_NO_THROW(source.invalidate());
		EXPECT_GT(source.version(), sourceVersion);
		EXPECT_EQ(destination.version(), destinationVersion);
	}

	TEST(VersionedTraitTest, DestructionInvalidatesOutstandingContracts)
	{
		VersionedTrait::Contract contract;

		{
			auto value = std::make_unique<VersionedTrait>();
			contract = value->subscribeToVersionEdition([](VersionedTrait *) {});
			ASSERT_TRUE(contract.isValid());
		}

		EXPECT_FALSE(contract.isValid());
	}

	TEST(VersionedTraitTest, DISABLED_VersionWrapRequiresDeterministicPublicSeam)
	{
		GTEST_SKIP() << "VersionedTrait exposes no public seam for placing its uint64_t version near UINT64_MAX; "
						 << "a deterministic wrap test cannot be written without relying on private state or billions of invalidations.";
	}
}
