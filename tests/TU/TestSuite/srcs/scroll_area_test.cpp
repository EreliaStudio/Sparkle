#include <gtest/gtest.h>

#include <stdexcept>

#include "ui/widget/scroll_area.hpp"

TEST(IScrollAreaTest, OversizedContentShowsBothBarsAndTracksOffsets)
{
	spk::IScrollArea area("Area");
	spk::Widget content("Content", &area.container());
	content.setMinimalSize({300, 200});
	area.setContent(&content);
	area.setGeometry({.anchor = {0, 0}, .size = {120, 90}});
	EXPECT_TRUE(area.isScrollBarVisible(spk::Orientation::Horizontal));
	EXPECT_TRUE(area.isScrollBarVisible(spk::Orientation::Vertical));
	EXPECT_EQ(area.viewSize(), spk::Vector2UInt(104, 74));
	EXPECT_EQ(area.contentSize(), spk::Vector2UInt(300, 200));
	area.horizontalScrollBar().setRatio(1.0f);
	area.verticalScrollBar().setRatio(0.5f);
	EXPECT_EQ(area.container().contentAnchor(), spk::Vector2Int(-196, -63));
}

TEST(IScrollAreaTest, ScrollBarDependencyAndDynamicHintsAreRecomputed)
{
	spk::IScrollArea area("Area");
	spk::Widget content("Content", &area.container());
	area.setGeometry({.anchor = {0, 0}, .size = {100, 100}});
	content.setMinimalSize({100, 101});
	area.setContent(&content);
	EXPECT_TRUE(area.isScrollBarVisible(spk::Orientation::Vertical));
	EXPECT_TRUE(area.isScrollBarVisible(spk::Orientation::Horizontal));

	content.setMinimalSize({20, 20});
	EXPECT_FALSE(area.isScrollBarVisible(spk::Orientation::Vertical));
	EXPECT_FALSE(area.isScrollBarVisible(spk::Orientation::Horizontal));
	EXPECT_EQ(area.viewSize(), spk::Vector2UInt(100, 100));
	EXPECT_EQ(area.container().contentAnchor(), spk::Vector2Int(0, 0));
}

TEST(IScrollAreaTest, VisibilityMatrixCoversIndependentAndDependentBars)
{
	spk::IScrollArea area("Area");
	spk::Widget content("Content", &area.container());
	area.setGeometry({.anchor = {0, 0}, .size = {100, 100}});
	area.setContent(&content);

	content.setMinimalSize({101, 20});
	EXPECT_TRUE(area.isScrollBarVisible(spk::Orientation::Horizontal));
	EXPECT_FALSE(area.isScrollBarVisible(spk::Orientation::Vertical));
	content.setMinimalSize({20, 101});
	EXPECT_FALSE(area.isScrollBarVisible(spk::Orientation::Horizontal));
	EXPECT_TRUE(area.isScrollBarVisible(spk::Orientation::Vertical));
	content.setMinimalSize({100, 101});
	EXPECT_TRUE(area.isScrollBarVisible(spk::Orientation::Horizontal));
	EXPECT_TRUE(area.isScrollBarVisible(spk::Orientation::Vertical));
}

TEST(IScrollAreaTest, ContentParentAndReplacementContractIsEnforced)
{
	spk::IScrollArea area("Area");
	spk::Widget first("First", &area.container());
	spk::Widget second("Second", &area.container());
	spk::Widget foreign("Foreign", nullptr);
	area.setContent(&first);
	area.setContent(&second);
	EXPECT_EQ(area.content(), &second);
	area.setContent(nullptr);
	EXPECT_EQ(area.content(), nullptr);
	EXPECT_THROW(area.setContent(&foreign), std::invalid_argument);
	area.setScrollBarWidth(7);
	EXPECT_EQ(area.scrollBarWidth(), 7u);

	const auto *iconset = spk::Widget::defaultStyle.get().iconset.get();
	ASSERT_NE(iconset, nullptr);
	spk::IScrollArea custom("Custom", iconset);
	EXPECT_EQ(custom.horizontalScrollBar().iconset(), iconset);
	EXPECT_EQ(custom.verticalScrollBar().iconset(), iconset);
}
