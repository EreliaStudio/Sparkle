#include <gtest/gtest.h>

#include <type_traits>

#include "ui/widget.hpp"

namespace
{
	static_assert(!std::is_copy_constructible_v<spk::Widget::Style>);
	static_assert(!std::is_copy_assignable_v<spk::Widget::Style>);
	static_assert(std::is_nothrow_move_constructible_v<spk::Widget::Style>);
	static_assert(std::is_nothrow_move_assignable_v<spk::Widget::Style>);
}

TEST(WidgetStyleTest, DefaultResourcesAndCoreValuesAreValid)
{
	const auto &style = spk::Widget::defaultStyle.get();
	const auto expectColor = [](const spk::Color &color) {
		EXPECT_GE(color.r, 0.0f); EXPECT_LE(color.r, 1.0f);
		EXPECT_GE(color.g, 0.0f); EXPECT_LE(color.g, 1.0f);
		EXPECT_GE(color.b, 0.0f); EXPECT_LE(color.b, 1.0f);
		EXPECT_GE(color.a, 0.0f); EXPECT_LE(color.a, 1.0f);
	};
	EXPECT_NE(style.font, nullptr);
	EXPECT_NE(style.iconsetImage, nullptr);
	EXPECT_NE(style.iconset, nullptr);
	EXPECT_NE(style.nineSlice, nullptr);
	EXPECT_NE(style.darkNineSlice, nullptr);
	EXPECT_NE(style.darkerNineSlice, nullptr);
	EXPECT_NE(style.lightNineSlice, nullptr);
	EXPECT_NE(style.sliderBody, nullptr);
	EXPECT_NE(style.menuBreak, nullptr);
	EXPECT_NE(style.toggleSwitchOutline, nullptr);
	EXPECT_NE(style.toggleSwitchThumb, nullptr);
	EXPECT_NE(style.toggleSwitchOffBackground, nullptr);
	EXPECT_NE(style.toggleSwitchOnBackground, nullptr);
	EXPECT_GT(style.textLabelTextSize.glyph, 0u);
	EXPECT_GT(style.pushButtonTextSize.glyph, 0u);
	EXPECT_GT(style.textEditTextSize.glyph, 0u);
	EXPECT_GT(style.interfaceWindowMenuTitleTextSize.glyph, 0u);
	EXPECT_GT(style.iconButtonIconSize.x, 0u);
	EXPECT_GT(style.iconButtonIconSize.y, 0u);
	EXPECT_GT(style.iconButtonIconPadding.x, 0u);
	EXPECT_GE(style.pushButtonCornerSize.x, 0);
	EXPECT_GE(style.textEditCornerSize.x, 0);
	EXPECT_GE(style.sliderBarBackgroundCornerSize.x, 0);
	EXPECT_GE(style.sliderBarBodyCornerSize.y, 0);
	EXPECT_GT(style.scrollBarButtonIconSize.x, 0u);
	EXPECT_GE(style.interfaceWindowBackgroundCornerSize.x, 0);
	EXPECT_GT(style.interfaceWindowMenuButtonIconSize.x, 0u);
	EXPECT_GT(style.interfaceWindowMenuButtonIconPadding.y, 0u);
	EXPECT_GE(style.interfaceWindowMenuButtonCornerSize.x, 0);
	expectColor(style.textLabelGlyphColor);
	expectColor(style.textLabelOutlineColor);
	expectColor(style.pushButtonReleasedGlyphColor);
	expectColor(style.pushButtonPressedGlyphColor);
	expectColor(style.textEditGlyphColor);
	expectColor(style.textEditOutlineColor);
	expectColor(style.textEditCursorColor);
	expectColor(style.interfaceWindowMenuTitleGlyphColor);
	expectColor(style.interfaceWindowMenuTitleOutlineColor);
}

TEST(WidgetStyleTest, MoveConstructionAndAssignmentTransferOwnedResources)
{
	spk::Widget::Style source;
	auto *font = source.font.get();
	auto *iconset = source.iconset.get();
	spk::Widget::Style moved(std::move(source));
	EXPECT_EQ(moved.font.get(), font);
	EXPECT_EQ(moved.iconset.get(), iconset);
	EXPECT_EQ(source.font, nullptr);
	spk::Widget::Style destination;
	destination = std::move(moved);
	EXPECT_EQ(destination.font.get(), font);
	EXPECT_EQ(destination.iconset.get(), iconset);
	EXPECT_EQ(moved.font, nullptr);
}
