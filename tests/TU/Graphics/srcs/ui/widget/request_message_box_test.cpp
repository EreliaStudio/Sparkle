#include <gtest/gtest.h>

#include "ui/widget/message_box.hpp"

namespace
{
	void click(spk::PushButton &button)
	{
		spk::Mouse mouse;
		mouse.position = button.viewRegion().viewport.anchor + spk::Vector2Int{1, 1};
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

TEST(RequestMessageBoxTest, ConfigureReplacesCaptionsAndBothActions)
{
	spk::RequestMessageBox box("Request");
	box.setGeometry({.anchor = {0, 0}, .size = {400, 200}});
	box.activate();
	int first = 0;
	int second = 0;
	box.configure("Proceed", [&]() { ++first; }, "Cancel", [&]() { ++second; });
	EXPECT_EQ(box.firstButton().releasedLabel().text(), U"Proceed");
	EXPECT_EQ(box.secondButton().releasedLabel().text(), U"Cancel");
	click(box.firstButton());
	click(box.secondButton());
	EXPECT_EQ(first, 1);
	EXPECT_EQ(second, 1);
}

TEST(RequestMessageBoxTest, TitleClosePolicyCanOverrideAndReset)
{
	spk::RequestMessageBox box("Request");
	box.setGeometry({.anchor = {0, 0}, .size = {400, 200}});
	box.activate();
	int second = 0;
	int custom = 0;
	box.configure("Yes", {}, "No", [&]() { ++second; });
	EXPECT_TRUE(box.usesSecondActionOnTitleClose());
	click(box.menuBar().closeButton());
	EXPECT_EQ(second, 1);
	box.setTitleCloseAction([&]() { ++custom; });
	EXPECT_FALSE(box.usesSecondActionOnTitleClose());
	click(box.menuBar().closeButton());
	EXPECT_EQ(custom, 1);
	box.resetTitleCloseAction();
	EXPECT_TRUE(box.usesSecondActionOnTitleClose());
	click(box.menuBar().closeButton());
	EXPECT_EQ(second, 2);
}
