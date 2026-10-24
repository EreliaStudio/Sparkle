#include <gtest/gtest.h>

#include <type_traits>

#include "ui/widget/scroll_area.hpp"
#include "ui/widget/spacer_widget.hpp"

namespace
{
	static_assert(spk::ScrollAreaContent<spk::Widget>);
	static_assert(spk::ScrollAreaContent<spk::SpacerWidget>);
}

TEST(ScrollAreaTemplateTest, ConstructsTypedContentWithCorrectParentAndAccessors)
{
	spk::ScrollArea<spk::Widget> widgets("Widgets");
	spk::ScrollArea<spk::SpacerWidget> spacers("Spacers");
	EXPECT_EQ(widgets.content(), &widgets.contentObject());
	EXPECT_EQ(widgets.contentObject().parent(), &widgets.container());
	EXPECT_EQ(spacers.content(), &spacers.contentObject());
	EXPECT_TRUE(spacers.contentObject().isActive());
	const auto &constant = std::as_const(widgets);
	EXPECT_EQ(&constant.contentObject(), &widgets.contentObject());
}

TEST(ScrollAreaTemplateTest, DefaultAndExplicitIconsetConstructorsRemainScrollable)
{
	const auto *iconset = spk::Widget::defaultStyle.get().iconset.get();
	ASSERT_NE(iconset, nullptr);
	spk::ScrollArea<spk::Widget> standard("Standard");
	spk::ScrollArea<spk::Widget> explicitIcons("Explicit", iconset);
	for (auto *area : {&standard, &explicitIcons})
	{
		area->contentObject().setMinimalSize({200, 150});
		area->setGeometry({.anchor = {0, 0}, .size = {100, 80}});
		EXPECT_TRUE(area->isScrollBarVisible(spk::Orientation::Horizontal));
		EXPECT_TRUE(area->isScrollBarVisible(spk::Orientation::Vertical));
	}
}

TEST(ScrollAreaTemplateTest, MissingIconsetIsRejectedAndDefaultIconsRemainScrollable)
{
	EXPECT_THROW((spk::ScrollArea<spk::Widget>("Absent", static_cast<const spk::SpriteSheet *>(nullptr))), std::invalid_argument);
	spk::ScrollArea<spk::Widget> area("Default");
	area.contentObject().setMinimalSize({300, 200});
	area.setGeometry({.anchor = {0, 0}, .size = {100, 80}});
	area.horizontalScrollBar().setRatio(1);
	area.verticalScrollBar().setRatio(1);
	EXPECT_LT(area.contentObject().geometry().x, 0);
	EXPECT_LT(area.contentObject().geometry().y, 0);
}
