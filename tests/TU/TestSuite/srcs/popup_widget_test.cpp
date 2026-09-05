#include <gtest/gtest.h>

#include <stdexcept>

#include "ui/widget/popup_widget.hpp"

TEST(PopupWidgetTest, AnchoredAndExplicitOpeningPlaceContentAndCloseOnce)
{
	spk::Widget root("Root", nullptr);
	root.setGeometry({.anchor = {0, 0}, .size = {300, 200}});
	spk::Widget anchor("Anchor", &root);
	anchor.setGeometry({.anchor = {100, 50}, .size = {40, 20}});
	spk::PopupWidget popup("Popup", &root);
	spk::Widget content("Content", &popup);
	content.setPreferredSize({80, 30});
	popup.setContent(&content);
	popup.setAnchorWidget(&anchor);
	popup.setPlacement({spk::Alignment::Horizontal::Right, spk::Alignment::Vertical::Bottom});
	int closes = 0;
	auto contract = popup.subscribeToClose([&]() { ++closes; });
	popup.open();
	EXPECT_TRUE(popup.isOpen());
	EXPECT_EQ(popup.geometry(), (spk::Rect2D{.anchor = {140, 70}, .size = {80, 30}}));
	EXPECT_EQ(content.geometry().size, spk::Vector2UInt(80, 30));
	popup.close();
	popup.close();
	EXPECT_EQ(closes, 1);
	popup.openAt({290, 190});
	EXPECT_EQ(popup.geometry().anchor, spk::Vector2Int(220, 170));
}

TEST(PopupWidgetTest, MissingAnchorAndForeignContentThrowWithoutOpening)
{
	spk::Widget root("Root", nullptr);
	root.setGeometry({.anchor = {0, 0}, .size = {100, 100}});
	spk::PopupWidget popup("Popup", &root);
	spk::Widget foreign("Foreign", &root);
	EXPECT_THROW(popup.setContent(&foreign), std::invalid_argument);
	EXPECT_THROW(popup.open(), std::logic_error);
	EXPECT_FALSE(popup.isOpen());
	EXPECT_EQ(popup.content(), nullptr);
}

TEST(PopupWidgetTest, OnePopupPerRootAndReparentedContentCleanupAreCoherent)
{
	spk::Widget root("Root", nullptr);
	root.setGeometry({.anchor = {0, 0}, .size = {200, 100}});
	spk::PopupWidget first("First", &root);
	spk::PopupWidget second("Second", &root);
	first.openAt({0, 0});
	second.openAt({10, 10});
	EXPECT_FALSE(first.isOpen());
	EXPECT_TRUE(second.isOpen());
	spk::Widget content("Content", &second);
	second.setContent(&content);
	content.setParent(&root);
	EXPECT_EQ(second.content(), nullptr);
}
