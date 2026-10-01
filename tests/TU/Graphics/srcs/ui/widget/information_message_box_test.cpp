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

TEST(InformationMessageBoxTest, DefaultCloseButtonCaptionAndAccessorsAreConfigured)
{
	spk::InformationMessageBox box("Information");
	box.setGeometry({.anchor = {0, 0}, .size = {300, 160}});
	box.activate();
	EXPECT_EQ(box.nbButton(), 1u);
	EXPECT_EQ(&box.closeButton(), &box.button("close"));
	EXPECT_EQ(box.closeButton().releasedLabel().text(), U"Close");
	EXPECT_EQ(box.menuBar().titleLabel().text(), U"Information");
	box.setText("Details");
	EXPECT_EQ(box.text(), U"Details");
	EXPECT_GT(box.minimumContentSize().y, 0.0f);
	click(box.closeButton());
	EXPECT_FALSE(box.isActive());
	click(box.closeButton());
	EXPECT_FALSE(box.isActive());
	const auto &constant = std::as_const(box);
	EXPECT_EQ(&constant.closeButton(), &box.closeButton());
}
