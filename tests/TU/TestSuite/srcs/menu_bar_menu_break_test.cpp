#include <gtest/gtest.h>

#include "ui/widget/menu_bar.hpp"

namespace
{
	const spk::Widget::Style &defaultStyle()
	{
		return spk::Widget::defaultStyle.get();
	}
}

TEST(MenuBarBreakTest, DefaultConfiguredAndStyleState)
{
	spk::MenuBar::Menu::Break separator("Break");
	EXPECT_EQ(separator.spriteSheet(), defaultStyle().menuBreak.get());
	EXPECT_EQ(separator.height(), 2u);
	EXPECT_FLOAT_EQ(separator.depth(), 0.0f);
	separator.setHeight(5);
	separator.setDepth(2.0f);
	separator.applyStyle(defaultStyle());
	EXPECT_EQ(separator.spriteSheet(), defaultStyle().menuBreak.get());
	EXPECT_EQ(separator.height(), 5u);
	EXPECT_FLOAT_EQ(separator.depth(), 2.0f);
}

TEST(MenuBarBreakTest, InvalidSpriteSheetsAreRejected)
{
	spk::MenuBar::Menu::Break separator("Break");
	EXPECT_THROW(separator.setSpriteSheet(nullptr), std::invalid_argument);
	EXPECT_THROW(separator.setSpriteSheet(defaultStyle().iconset.get()), std::invalid_argument);
}

TEST(MenuBarBreakRenderTest, DISABLED_ThreePartSeparator)
{
	// Requires the separator golden image.
}
