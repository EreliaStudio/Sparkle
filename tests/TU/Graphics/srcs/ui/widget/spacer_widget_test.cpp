#include <gtest/gtest.h>

#include <limits>

#include "ui/layout/linear_layout.hpp"
#include "ui/widget/spacer_widget.hpp"

TEST(SpacerWidgetTest, DefaultStateIsActiveAndExpandsWithoutVisualContent)
{
	spk::SpacerWidget spacer("Spacer");

	EXPECT_TRUE(spacer.isActive());
	EXPECT_EQ(spacer.minimalSize(), spk::Vector2(0, 0));
	EXPECT_EQ(spacer.preferredSize(), spk::Vector2(0, 0));
	EXPECT_EQ(spacer.maximalSize(), (spk::Vector2{
		std::numeric_limits<float>::max(), std::numeric_limits<float>::max()}));
	EXPECT_TRUE(spacer.children().empty());
}

TEST(SpacerWidgetTest, HorizontalLayoutAssignsRemainingWidth)
{
	spk::SpacerWidget spacer("Spacer");
	spk::Widget fixed("Fixed", nullptr);
	spacer.setSizeHint({{10, 12}, {200, 40}, {20, 18}});
	fixed.setSizeHint({{30, 18}, {30, 18}, {30, 18}});

	spk::HorizontalLayout layout;
	layout.addWidget(&fixed, spk::Layout::SizePolicy::Fixed);
	layout.addWidget(&spacer);
	layout.setGeometry({.anchor = {5, 7}, .size = {100, 20}});

	EXPECT_EQ(fixed.geometry(), (spk::Rect2D{.anchor = {5, 7}, .size = {30, 18}}));
	EXPECT_EQ(spacer.geometry(), (spk::Rect2D{.anchor = {35, 7}, .size = {70, 20}}));
}

TEST(SpacerWidgetTest, VerticalLayoutAssignsRemainingHeightAndHonorsMinimum)
{
	spk::SpacerWidget spacer("Spacer");
	spk::Widget fixed("Fixed", nullptr);
	spacer.setSizeHint({{14, 15}, {50, 300}, {20, 25}});
	fixed.setSizeHint({{20, 30}, {20, 30}, {20, 30}});

	spk::VerticalLayout layout;
	layout.addWidget(&fixed, spk::Layout::SizePolicy::Fixed);
	layout.addWidget(&spacer);
	layout.setGeometry({.anchor = {3, 4}, .size = {24, 120}});

	EXPECT_EQ(fixed.geometry(), (spk::Rect2D{.anchor = {3, 4}, .size = {20, 30}}));
	EXPECT_EQ(spacer.geometry(), (spk::Rect2D{.anchor = {3, 34}, .size = {24, 90}}));

	layout.setGeometry({.anchor = {0, 0}, .size = {10, 35}});
	EXPECT_LE(spacer.geometry().height, 15u);
	EXPECT_EQ(fixed.geometry().height + spacer.geometry().height, 35u);
}
