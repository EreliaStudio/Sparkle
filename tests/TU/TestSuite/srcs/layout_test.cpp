#include <gtest/gtest.h>

#include "ui/layout/linear_layout.hpp"
#include "ui/widget.hpp"

namespace
{
	spk::ResizeableTrait::SizeHint hint(spk::Vector2 minimum, spk::Vector2 maximum, spk::Vector2 preferred)
	{
		return {.minimal = minimum, .maximal = maximum, .preferred = preferred};
	}
}

TEST(LayoutTest, ElementExposesKindPoliciesHintsAndEveryAlignment)
{
	spk::HorizontalLayout layout;
	spk::Widget widget("widget", nullptr);
	widget.setSizeHint(hint({10, 10}, {100, 80}, {30, 20}));
	auto *element = layout.addWidget(&widget, {spk::Layout::SizePolicy::Fixed, spk::Layout::SizePolicy::Minimum});
	EXPECT_TRUE(element->isWidget());
	EXPECT_FALSE(element->isLayout());
	EXPECT_EQ(element->widget(), &widget);
	EXPECT_EQ(element->layout(), nullptr);
	EXPECT_EQ(element->minimalSize(), spk::Vector2(30, 10));
	EXPECT_EQ(element->preferredSize(), spk::Vector2(30, 10));
	EXPECT_EQ(element->maximalSize(), spk::Vector2(30, 10));

	element->setAlignment({spk::Alignment::Horizontal::Right, spk::Alignment::Vertical::Bottom});
	EXPECT_EQ(element->alignment(), (spk::Alignment{spk::Alignment::Horizontal::Right, spk::Alignment::Vertical::Bottom}));
	element->setGeometry({.anchor = {0, 0}, .size = {100, 80}});
	EXPECT_EQ(widget.geometry(), (spk::Rect2D{.anchor = {70, 70}, .size = {30, 10}}));
	element->setHorizontalAlignment(spk::Alignment::Horizontal::Center);
	element->setVerticalAlignment(spk::Alignment::Vertical::Center);
	EXPECT_EQ(element->horizontalAlignment(), spk::Alignment::Horizontal::Center);
	EXPECT_EQ(element->verticalAlignment(), spk::Alignment::Vertical::Center);
}

TEST(LayoutTest, NestedLayoutAndChildHintEditsUpdateComputedHint)
{
	spk::HorizontalLayout outer;
	spk::VerticalLayout inner;
	spk::Widget widget("widget", nullptr);
	widget.setSizeHint(hint({5, 6}, {50, 60}, {20, 30}));
	inner.addWidget(&widget);
	auto *element = outer.addLayout(&inner);
	EXPECT_TRUE(element->isLayout());
	EXPECT_EQ(element->layout(), &inner);
	EXPECT_EQ(outer.preferredSize(), spk::Vector2(20, 30));
	widget.setPreferredSize({25, 35});
	EXPECT_EQ(outer.preferredSize(), spk::Vector2(25, 35));
}

TEST(LayoutTest, PaddingClearAndInvalidChildrenHaveDefinedBehavior)
{
	spk::HorizontalLayout layout;
	spk::Widget widget("widget", nullptr);
	layout.addWidget(&widget);
	layout.setElementPadding({7, 9});
	EXPECT_EQ(layout.elementPadding(), spk::Vector2UInt(7, 9));
	EXPECT_EQ(layout.elements().size(), 1u);
	EXPECT_THROW(layout.addWidget(nullptr), std::invalid_argument);
	EXPECT_THROW(layout.addLayout(nullptr), std::invalid_argument);
	EXPECT_THROW(layout.addLayout(&layout), std::invalid_argument);
	layout.clear();
	EXPECT_TRUE(layout.elements().empty());
	EXPECT_EQ(layout.sizeHint(), (spk::ResizeableTrait::SizeHint{}));
}
