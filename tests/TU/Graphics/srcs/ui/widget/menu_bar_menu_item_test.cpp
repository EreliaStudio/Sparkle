#include <gtest/gtest.h>

#include "ui/widget/menu_bar.hpp"

TEST(MenuBarMenuItemTest, ClickInvokesActionAndClosesOwnerMenu)
{
	spk::MenuBar::Menu menu("Menu");
	int calls = 0;
	auto &item = menu.addItem("open", "Open", [&] {
		++calls;
	});
	menu.setGeometry({.anchor = {0, 0}, .size = {160, 80}});
	menu.activate();
	item.setGeometry({.anchor = {0, 0}, .size = {100, 30}});
	item.activate();
	spk::Mouse mouse;
	mouse.position = {10, 10};
	spk::MouseButtonPressedRecord pressedRecord{};
	pressedRecord.button = spk::Mouse::Button::Left;
	spk::MouseButtonPressedEvent pressed(pressedRecord, mouse);
	item.dispatch(pressed);
	spk::MouseButtonReleasedRecord releasedRecord{};
	releasedRecord.button = spk::Mouse::Button::Left;
	spk::MouseButtonReleasedEvent released(releasedRecord, mouse);
	item.dispatch(released);
	EXPECT_EQ(calls, 1);
	EXPECT_FALSE(menu.isActive());
}

TEST(MenuBarMenuItemTest, EmptyActionStillClosesOwner)
{
	spk::MenuBar::Menu menu("Menu");
	auto &item = menu.addItem("noop", "No action");
	menu.activate();
	item.setGeometry({.anchor = {0, 0}, .size = {100, 30}});
	spk::Mouse mouse;
	mouse.position = {10, 10};
	spk::MouseButtonPressedRecord p{};
	p.button = spk::Mouse::Button::Left;
	spk::MouseButtonPressedEvent pressed(p, mouse);
	item.dispatch(pressed);
	spk::MouseButtonReleasedRecord r{};
	r.button = spk::Mouse::Button::Left;
	spk::MouseButtonReleasedEvent released(r, mouse);
	item.dispatch(released);
	EXPECT_FALSE(menu.isActive());
}

TEST(MenuBarMenuItemTest, OwnerRetainsAndReleasesActionLifetime)
{
	spk::MenuBar::Menu menu("Menu");
	auto token = std::make_shared<int>(42);
	std::weak_ptr<int> lifetime = token;
	menu.addItem("owned", "Owned", [token] {
	});
	token.reset();
	EXPECT_FALSE(lifetime.expired());
	menu.clear();
	EXPECT_TRUE(lifetime.expired());
}
