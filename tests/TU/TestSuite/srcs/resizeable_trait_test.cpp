#include <gtest/gtest.h>

#include "design_pattern/trait/resizeable_trait.hpp"

#include <limits>
#include <utility>

TEST(ResizeableTraitTest, StandardUsageUpdatesCompleteAndIndividualHintsAndNotifies)
{
	spk::ResizeableTrait trait;
	int notificationCount = 0;
	spk::ResizeableTrait *lastSender = nullptr;

	auto contract = trait.subscribeToSizeHintEdition(
		[&](spk::ResizeableTrait *sender) {
			++notificationCount;
			lastSender = sender;
		});

	const spk::ResizeableTrait::SizeHint complete{
		.minimal = {1.0f, 2.0f},
		.maximal = {100.0f, 200.0f},
		.preferred = {30.0f, 40.0f}};

	trait.setSizeHint(complete);
	EXPECT_EQ(trait.sizeHint(), complete);
	EXPECT_EQ(notificationCount, 1);
	EXPECT_EQ(lastSender, &trait);

	trait.setMinimalSize({3.0f, 4.0f});
	EXPECT_EQ(trait.minimalSize(), (spk::Vector2{3.0f, 4.0f}));
	EXPECT_EQ(notificationCount, 2);

	trait.setMaximalSize({90.0f, 180.0f});
	EXPECT_EQ(trait.maximalSize(), (spk::Vector2{90.0f, 180.0f}));
	EXPECT_EQ(notificationCount, 3);

	trait.setPreferredSize({25.0f, 35.0f});
	EXPECT_EQ(trait.preferredSize(), (spk::Vector2{25.0f, 35.0f}));
	EXPECT_EQ(notificationCount, 4);
}

TEST(ResizeableTraitTest, EachEffectiveSetterCallProducesOneNotification)
{
	spk::ResizeableTrait trait;
	int notificationCount = 0;

	auto contract = trait.subscribeToSizeHintEdition(
		[&](spk::ResizeableTrait *) {
			++notificationCount;
		});

	trait.setMinimalSize({1.0f, 1.0f});
	EXPECT_EQ(notificationCount, 1);

	trait.setMaximalSize({2.0f, 2.0f});
	EXPECT_EQ(notificationCount, 2);

	trait.setPreferredSize({1.5f, 1.5f});
	EXPECT_EQ(notificationCount, 3);

	trait.setSizeHint({
		.minimal = {3.0f, 3.0f},
		.maximal = {4.0f, 4.0f},
		.preferred = {3.5f, 3.5f}});
	EXPECT_EQ(notificationCount, 4);
}

TEST(ResizeableTraitTest, ZeroNegativeInfiniteAndContradictoryHintsAreAcceptedAsData)
{
	spk::ResizeableTrait trait;
	const float infinity = std::numeric_limits<float>::infinity();

	const spk::ResizeableTrait::SizeHint unusual{
		.minimal = {-10.0f, 0.0f},
		.maximal = {-20.0f, infinity},
		.preferred = {1000.0f, -infinity}};

	EXPECT_NO_THROW(trait.setSizeHint(unusual));
	EXPECT_EQ(trait.sizeHint(), unusual);

	EXPECT_NO_THROW(trait.setMinimalSize({0.0f, -5.0f}));
	EXPECT_EQ(trait.minimalSize(), (spk::Vector2{0.0f, -5.0f}));

	EXPECT_NO_THROW(trait.setMaximalSize({-100.0f, infinity}));
	EXPECT_EQ(trait.maximalSize(), (spk::Vector2{-100.0f, infinity}));

	EXPECT_NO_THROW(trait.setPreferredSize({infinity, -infinity}));
	EXPECT_EQ(trait.preferredSize(), (spk::Vector2{infinity, -infinity}));
}

TEST(ResizeableTraitTest, DestructorInvalidatesOutstandingContracts)
{
	spk::ResizeableTrait::Contract contract;

	{
		spk::ResizeableTrait trait;
		contract = trait.subscribeToSizeHintEdition([](spk::ResizeableTrait *) {});
		ASSERT_TRUE(contract.isValid());
	}

	EXPECT_FALSE(contract.isValid());
	EXPECT_NO_THROW(contract.resign());
}
