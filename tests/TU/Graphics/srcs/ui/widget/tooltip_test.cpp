#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

#include "ui/widget/tooltip.hpp"

TEST(TooltipTest, ManualLifecycleRequiresTargetAndTextAndCoordinatesOnePerRoot)
{
	spk::Widget root("Root", nullptr);
	root.setGeometry({.anchor = {0, 0}, .size = {240, 120}});
	spk::Widget target("Target", &root);
	target.setGeometry({.anchor = {20, 30}, .size = {40, 20}});
	spk::Tooltip first("First", &root);
	spk::Tooltip second("Second", &root);
	first.show();
	EXPECT_FALSE(first.isShown());
	first.setTarget(&target);
	first.setText("First tip");
	first.show();
	EXPECT_TRUE(first.isShown());
	EXPECT_TRUE(first.background().isActive());
	second.setTarget(&target);
	second.setText("Second tip");
	second.show();
	EXPECT_FALSE(first.isShown());
	EXPECT_TRUE(second.isShown());
	second.hide();
	second.hide();
	EXPECT_FALSE(second.isShown());
}

TEST(TooltipTest, PlacementsMaximumWidthAndTargetReplacementUpdateGeometry)
{
	spk::Widget root("Root", nullptr);
	root.setGeometry({.anchor = {0, 0}, .size = {160, 100}});
	spk::Widget first("First", &root);
	spk::Widget second("Second", &root);
	first.setGeometry({.anchor = {10, 50}, .size = {30, 10}});
	second.setGeometry({.anchor = {120, 5}, .size = {30, 10}});
	spk::Tooltip tooltip("Tooltip", &root);
	tooltip.setText("A long tooltip that must remain inside the root");
	tooltip.setMaximumWidth(80);
	tooltip.setPlacement(spk::Tooltip::Placement::AboveTarget);
	tooltip.setTarget(&first);
	tooltip.show();
	EXPECT_LE(tooltip.geometry().width, 80u);
	EXPECT_GE(tooltip.geometry().x, 0);
	EXPECT_GE(tooltip.geometry().y, 0);
	tooltip.setTarget(&second);
	EXPECT_FALSE(tooltip.isShown());
	EXPECT_EQ(tooltip.target(), &second);
}

TEST(TooltipTest, DelayValidationAndEmptyTextHideAreAtomic)
{
	using namespace std::chrono_literals;
	spk::Widget root("Root", nullptr);
	root.setGeometry({.anchor = {0, 0}, .size = {100, 100}});
	spk::Widget target("Target", &root);
	spk::Tooltip tooltip("Tooltip", &root);
	tooltip.setTarget(&target);
	tooltip.setText("Tip");
	tooltip.setOpenDelay(25ms);
	tooltip.setCloseDelay(40ms);
	EXPECT_EQ(tooltip.openDelay(), 25ms);
	EXPECT_EQ(tooltip.closeDelay(), 40ms);
	EXPECT_THROW(tooltip.setOpenDelay(-1ms), std::invalid_argument);
	EXPECT_THROW(tooltip.setCloseDelay(-1ms), std::invalid_argument);
	EXPECT_EQ(tooltip.openDelay(), 25ms);
	tooltip.show();
	tooltip.setText("");
	EXPECT_FALSE(tooltip.isShown());
}
