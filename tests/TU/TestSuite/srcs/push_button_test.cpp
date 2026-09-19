#include <gtest/gtest.h>

#include <stdexcept>

#include "ui/widget/push_button.hpp"

namespace
{
	void click(spk::PushButton &button, spk::Vector2Int position)
	{
		spk::Mouse mouse;
		mouse.position = position;
		spk::MouseButtonPressedRecord press{};
		press.button = spk::Mouse::Button::Left;
		spk::MouseButtonPressedEvent pressed(press, mouse);
		button.dispatch(pressed);
		spk::MouseButtonReleasedRecord release{};
		release.button = spk::Mouse::Button::Left;
		spk::MouseButtonReleasedEvent released(release, mouse);
		button.dispatch(released);
	}
}

TEST(PushButtonTest, StandardClickAndOutsideReleaseHaveDefinedState)
{
	spk::PushButton button("Button");
	button.setGeometry({.anchor = {0, 0}, .size = {120, 40}});
	int clicks = 0;
	auto contract = button.subscribeToClick([&]() { ++clicks; });
	click(button, {10, 10});
	EXPECT_EQ(clicks, 1);
	EXPECT_FALSE(button.isPressed());

	spk::Mouse mouse;
	mouse.position = {10, 10};
	spk::MouseButtonPressedRecord press{};
	press.button = spk::Mouse::Button::Left;
	spk::MouseButtonPressedEvent pressed(press, mouse);
	button.dispatch(pressed);
	mouse.position = {200, 200};
	spk::MouseButtonReleasedRecord release{};
	release.button = spk::Mouse::Button::Left;
	spk::MouseButtonReleasedEvent released(release, mouse);
	button.dispatch(released);
	EXPECT_EQ(clicks, 1);
}

TEST(PushButtonTest, TextIconAlignmentSizingAndFlatModeRoundTrip)
{
	spk::PushButton button("Button");
	const auto *icons = spk::Widget::defaultStyle.get().iconset.get();
	ASSERT_NE(icons, nullptr);
	button.setText("Launch");
	button.setIcon(icons, 1);
	button.setTextPadding({4, 5});
	button.setIconSize({20, 18});
	button.setIconPadding({2, 3});
	button.setAlignment({spk::Alignment::Horizontal::Left, spk::Alignment::Vertical::Top});
	button.setFlat(true);
	EXPECT_TRUE(button.hasIcon());
	EXPECT_TRUE(button.isFlat());
	EXPECT_EQ(button.releasedLabel().text(), U"Launch");
	EXPECT_EQ(button.textPadding(), spk::Vector2UInt(4, 5));
	EXPECT_EQ(button.iconSize(), spk::Vector2UInt(20, 18));
	EXPECT_EQ(button.iconPadding(), spk::Vector2UInt(2, 3));
	button.removeIcon();
	EXPECT_FALSE(button.hasIcon());
	button.resetTextPadding();
	button.resetIconSize();
	button.resetIconPadding();
	EXPECT_FALSE(button.textPadding().has_value());
}

TEST(PushButtonTest, InvalidIconsThrowWithoutReplacingValidIcon)
{
	spk::PushButton button("Button");
	const auto *icons = spk::Widget::defaultStyle.get().iconset.get();
	ASSERT_NE(icons, nullptr);
	button.setIcon(icons, 0);
	EXPECT_THROW(button.setIcon(static_cast<const spk::Texture *>(nullptr)), std::invalid_argument);
	EXPECT_THROW(button.setIcon(static_cast<const spk::SpriteSheet *>(nullptr), 0), std::invalid_argument);
	EXPECT_THROW(button.setIcon(icons, icons->sprites().size()), std::out_of_range);
	EXPECT_THROW(button.setIcon(icons, icons->nbSprite()), std::out_of_range);
	EXPECT_TRUE(button.hasIcon());
}
