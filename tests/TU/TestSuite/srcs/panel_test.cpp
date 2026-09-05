#include <gtest/gtest.h>

#include <stdexcept>

#include "ui/widget/panel.hpp"

TEST(PanelTest, DefaultStyleProvidesNineSliceAndIntrinsicCornerHint)
{
	spk::Panel panel("Panel");
	const auto &style = spk::Widget::defaultStyle.get();
	ASSERT_NE(style.nineSlice, nullptr);
	EXPECT_EQ(panel.spriteSheet(), style.nineSlice.get());
	EXPECT_EQ(panel.minimalSize(), spk::Vector2(panel.cornerSize().x * 2, panel.cornerSize().y * 2));
	EXPECT_EQ(panel.preferredSize(), panel.minimalSize());
	EXPECT_TRUE(panel.isActive());
}

TEST(PanelTest, ExplicitCornersDepthAndSmallGeometryRoundTrip)
{
	spk::Panel panel("Panel");
	panel.setCornerSize({0, 7});
	panel.setDepth(-3.25f);
	panel.setGeometry({.anchor = {10, 20}, .size = {5, 6}});
	EXPECT_EQ(panel.cornerSize(), spk::Vector2Int(0, 7));
	EXPECT_EQ(panel.minimalSize(), spk::Vector2(0, 14));
	EXPECT_FLOAT_EQ(panel.depth(), -3.25f);
	EXPECT_EQ(panel.geometry(), (spk::Rect2D{.anchor = {10, 20}, .size = {5, 6}}));
}

TEST(PanelTest, ResourceAndCornerFailuresPreservePriorState)
{
	spk::Panel panel("Panel");
	const auto *sheet = panel.spriteSheet();
	const auto corner = panel.cornerSize();
	EXPECT_THROW(panel.setSpriteSheet(nullptr), std::invalid_argument);
	EXPECT_THROW(panel.setCornerSize({-1, 4}), std::invalid_argument);
	EXPECT_THROW(panel.setCornerSize({4, -1}), std::invalid_argument);
	EXPECT_EQ(panel.spriteSheet(), sheet);
	EXPECT_EQ(panel.cornerSize(), corner);
}
