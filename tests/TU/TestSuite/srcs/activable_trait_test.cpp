#include <gtest/gtest.h>

#include "design_pattern/trait/activable_trait.hpp"

TEST(ActivableTraitTest, StandardUsageActivatesDeactivatesTogglesAndNotifies)
{
	spk::ActivableTrait trait;
	int activationCount = 0;
	int deactivationCount = 0;

	auto activationContract = trait.subscribeToActivation([&]() {
		++activationCount;
	});
	auto deactivationContract = trait.subscribeToDeactivation([&]() {
		++deactivationCount;
	});

	EXPECT_FALSE(trait.isActive());

	trait.activate();
	EXPECT_TRUE(trait.isActive());
	EXPECT_EQ(activationCount, 1);
	EXPECT_EQ(deactivationCount, 0);

	trait.toggle();
	EXPECT_FALSE(trait.isActive());
	EXPECT_EQ(activationCount, 1);
	EXPECT_EQ(deactivationCount, 1);

	trait.toggle();
	EXPECT_TRUE(trait.isActive());
	EXPECT_EQ(activationCount, 2);
	EXPECT_EQ(deactivationCount, 1);

	trait.deactivate();
	EXPECT_FALSE(trait.isActive());
	EXPECT_EQ(activationCount, 2);
	EXPECT_EQ(deactivationCount, 2);
}

TEST(ActivableTraitTest, DefaultInitialStatusIsDeactivated)
{
	const spk::ActivableTrait trait;
	EXPECT_FALSE(trait.isActive());
}

TEST(ActivableTraitTest, ExplicitInitialStatusIsRespected)
{
	const spk::ActivableTrait active(spk::ActivationStatus::Activated);
	const spk::ActivableTrait inactive(spk::ActivationStatus::Deactivated);

	EXPECT_TRUE(active.isActive());
	EXPECT_FALSE(inactive.isActive());
}

TEST(ActivableTraitTest, RepeatedTransitionsToCurrentStateAreNoOps)
{
	spk::ActivableTrait trait;
	int activationCount = 0;
	int deactivationCount = 0;

	auto activationContract = trait.subscribeToActivation([&]() {
		++activationCount;
	});
	auto deactivationContract = trait.subscribeToDeactivation([&]() {
		++deactivationCount;
	});

	trait.deactivate();
	trait.deactivate();
	EXPECT_EQ(deactivationCount, 0);

	trait.activate();
	trait.activate();
	trait.activate();
	EXPECT_EQ(activationCount, 1);

	trait.deactivate();
	trait.deactivate();
	EXPECT_EQ(deactivationCount, 1);
}

TEST(ActivableTraitTest, ActivationCallbackCanTriggerDeactivation)
{
	spk::ActivableTrait trait;
	int activationCount = 0;
	int deactivationCount = 0;

	auto deactivationContract = trait.subscribeToDeactivation([&]() {
		++deactivationCount;
	});
	auto activationContract = trait.subscribeToActivation([&]() {
		++activationCount;
		trait.deactivate();
	});

	trait.activate();

	EXPECT_FALSE(trait.isActive());
	EXPECT_EQ(activationCount, 1);
	EXPECT_EQ(deactivationCount, 1);
}

TEST(ActivableTraitTest, DeactivationCallbackCanTriggerActivation)
{
	spk::ActivableTrait trait(spk::ActivationStatus::Activated);
	int activationCount = 0;
	int deactivationCount = 0;

	auto activationContract = trait.subscribeToActivation([&]() {
		++activationCount;
	});
	auto deactivationContract = trait.subscribeToDeactivation([&]() {
		++deactivationCount;
		trait.activate();
	});

	trait.deactivate();

	EXPECT_TRUE(trait.isActive());
	EXPECT_EQ(deactivationCount, 1);
	EXPECT_EQ(activationCount, 1);
}
