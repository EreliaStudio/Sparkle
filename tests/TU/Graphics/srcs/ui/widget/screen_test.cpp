#include <gtest/gtest.h>

#include "ui/widget/screen.hpp"

TEST(ScreenTest, ActivatingScreensMaintainsOneGlobalActiveScreen)
{
	spk::Screen first("First");
	spk::Screen second("Second");
	int firstDeactivations = 0;
	int secondActivations = 0;
	auto firstContract = first.subscribeToDeactivation([&]() { ++firstDeactivations; });
	auto secondContract = second.subscribeToActivation([&]() { ++secondActivations; });

	EXPECT_EQ(spk::Screen::activeScreen(), nullptr);
	first.activate();
	EXPECT_EQ(spk::Screen::activeScreen(), &first);
	EXPECT_TRUE(first.isActive());
	EXPECT_FALSE(second.isActive());

	second.activate();
	EXPECT_EQ(spk::Screen::activeScreen(), &second);
	EXPECT_FALSE(first.isActive());
	EXPECT_TRUE(second.isActive());
	EXPECT_EQ(firstDeactivations, 1);
	EXPECT_EQ(secondActivations, 1);

	second.activate();
	EXPECT_EQ(secondActivations, 1);
}

TEST(ScreenTest, DestructionClearsOnlyTheActiveScreen)
{
	spk::Screen inactive("Inactive");
	{
		spk::Screen active("Active");
		active.activate();
		EXPECT_EQ(spk::Screen::activeScreen(), &active);
	}
	EXPECT_EQ(spk::Screen::activeScreen(), nullptr);

	{
		spk::Screen active("Other active");
		active.activate();
		{
			spk::Screen anotherInactive("Another inactive");
		}
		EXPECT_EQ(spk::Screen::activeScreen(), &active);
	}
	EXPECT_EQ(spk::Screen::activeScreen(), nullptr);
}

TEST(ScreenTest, ActivationIsGlobalAcrossParentHierarchies)
{
	spk::Widget firstRoot("First root", nullptr);
	spk::Widget secondRoot("Second root", nullptr);
	spk::Screen nested("Nested", &firstRoot);
	spk::Screen independent("Independent", &secondRoot);

	nested.activate();
	EXPECT_EQ(spk::Screen::activeScreen(), &nested);
	independent.activate();

	EXPECT_EQ(spk::Screen::activeScreen(), &independent);
	EXPECT_FALSE(nested.isActive());
	EXPECT_TRUE(independent.isActive());
}
