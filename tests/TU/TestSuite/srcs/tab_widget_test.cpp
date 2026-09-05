#include <gtest/gtest.h>

#include <stdexcept>

#include "ui/widget/tab_widget.hpp"

TEST(TabWidgetTest, PagesSelectRemoveAndExposeActiveContent)
{
	spk::TabWidget tabs("Tabs");
	spk::Widget first("First", &tabs);
	spk::Widget second("Second", &tabs);
	tabs.addPage("One", &first);
	tabs.addPage("Two", &second);
	EXPECT_EQ(tabs.pageCount(), 2u);
	EXPECT_EQ(tabs.selectedIndex(), 0u);
	EXPECT_EQ(tabs.selectedPage(), &first);
	EXPECT_TRUE(first.isActive());
	EXPECT_FALSE(second.isActive());

	int editions = 0;
	auto contract = tabs.subscribeToSelection([&](std::size_t index) {
		++editions;
		EXPECT_EQ(index, 1u);
	});
	tabs.selectPage(1);
	tabs.selectPage(1);
	EXPECT_EQ(editions, 1);
	EXPECT_FALSE(first.isActive());
	EXPECT_TRUE(second.isActive());
	tabs.removePage(&first);
	EXPECT_EQ(tabs.selectedIndex(), 0u);
	EXPECT_EQ(tabs.selectedPage(), &second);
}

TEST(TabWidgetTest, SizingPaddingAndEmptyStateUpdateGeometry)
{
	spk::TabWidget tabs("Tabs");
	spk::Widget page("Page", &tabs);
	page.setSizeHint({{80, 40}, {400, 300}, {120, 70}});
	tabs.addPage("Page", &page);
	tabs.setTabBarHeight(30);
	tabs.setTabWidth(90);
	tabs.setContentTopPadding(5);
	tabs.setGeometry({.anchor = {0, 0}, .size = {200, 100}});
	EXPECT_EQ(tabs.tabButton(0).geometry(), (spk::Rect2D{.anchor = {0, 0}, .size = {90, 30}}));
	EXPECT_EQ(page.geometry(), (spk::Rect2D{.anchor = {0, 35}, .size = {200, 65}}));
	EXPECT_EQ(tabs.minimalSize(), spk::Vector2(90, 75));
	tabs.removePage(&page);
	EXPECT_EQ(tabs.pageCount(), 0u);
	EXPECT_FALSE(tabs.selectedIndex().has_value());
	EXPECT_EQ(tabs.selectedPage(), nullptr);
}

TEST(TabWidgetTest, InvalidPagesDuplicatesAndIndicesAreAtomic)
{
	spk::TabWidget tabs("Tabs");
	spk::Widget child("Child", &tabs);
	spk::Widget foreign("Foreign", nullptr);
	EXPECT_THROW(tabs.addPage("Null", nullptr), std::invalid_argument);
	EXPECT_THROW(tabs.addPage("Foreign", &foreign), std::invalid_argument);
	tabs.addPage("Child", &child);
	EXPECT_THROW(tabs.addPage("Duplicate", &child), std::invalid_argument);
	EXPECT_THROW(tabs.selectPage(1), std::out_of_range);
	EXPECT_THROW((void)tabs.page(1), std::out_of_range);
	EXPECT_THROW((void)tabs.tabButton(1), std::out_of_range);
	EXPECT_EQ(tabs.pageCount(), 1u);
}
