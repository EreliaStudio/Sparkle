#include <gtest/gtest.h>

#include "ui/layout/linear_layout.hpp"
#include "ui/widget.hpp"

namespace
{
	void setHint(spk::Widget &widget, float minimum, float preferred, float maximum)
	{
		widget.setSizeHint({.minimal = {minimum, 10}, .maximal = {maximum, 50}, .preferred = {preferred, 20}});
	}
}

TEST(LinearLayoutTest, HorizontalMixedPoliciesDistributeSpaceWithPadding)
{
	spk::HorizontalLayout layout;
	spk::Widget fixed("fixed", nullptr);
	spk::Widget extending("extending", nullptr);
	setHint(fixed, 10, 20, 40);
	setHint(extending, 10, 30, 100);
	layout.addWidget(&fixed, spk::Layout::SizePolicy::Fixed);
	layout.addWidget(&extending);
	layout.setElementPadding({5, 0});
	layout.setGeometry({.anchor = {10, 20}, .size = {100, 40}});
	EXPECT_EQ(fixed.geometry(), (spk::Rect2D{.anchor = {10, 20}, .size = {20, 20}}));
	EXPECT_EQ(extending.geometry(), (spk::Rect2D{.anchor = {35, 20}, .size = {75, 40}}));
}

TEST(LinearLayoutTest, VerticalLayoutUsesVerticalHintsAndAlignment)
{
	spk::VerticalLayout layout;
	spk::Widget first("first", nullptr);
	spk::Widget second("second", nullptr);
	first.setSizeHint({.minimal = {10, 10}, .maximal = {30, 40}, .preferred = {20, 20}});
	second.setSizeHint({.minimal = {10, 10}, .maximal = {50, 80}, .preferred = {20, 30}});
	auto *firstElement = layout.addWidget(&first, spk::Layout::SizePolicy::Fixed);
	firstElement->setHorizontalAlignment(spk::Alignment::Horizontal::Center);
	layout.addWidget(&second);
	layout.setElementPadding({0, 4});
	layout.setGeometry({.anchor = {0, 0}, .size = {60, 104}});
	EXPECT_EQ(first.geometry(), (spk::Rect2D{.anchor = {20, 0}, .size = {20, 20}}));
	EXPECT_EQ(second.geometry(), (spk::Rect2D{.anchor = {0, 24}, .size = {50, 80}}));
}

TEST(LinearLayoutTest, RemovePresentAbsentAndForeignElementsIsSafe)
{
	spk::HorizontalLayout layout;
	spk::HorizontalLayout foreign;
	spk::Widget first("first", nullptr);
	spk::Widget second("second", nullptr);
	spk::Widget absent("absent", nullptr);
	auto *firstElement = layout.addWidget(&first);
	auto *foreignElement = foreign.addWidget(&absent);
	layout.addWidget(&second);
	layout.removeElement(foreignElement);
	layout.removeWidget(&absent);
	layout.removeLayout(&foreign);
	EXPECT_EQ(layout.elements().size(), 2u);
	layout.removeElement(firstElement);
	EXPECT_EQ(layout.elements().size(), 1u);
	layout.removeWidget(&second);
	EXPECT_TRUE(layout.elements().empty());
}

TEST(LinearLayoutTest, PixelRoundingUsesAllSpaceWithoutGapsOrOverlap)
{
	spk::HorizontalLayout layout;
	spk::Widget a("a", nullptr), b("b", nullptr), c("c", nullptr);
	for (auto *widget : {&a, &b, &c})
		widget->setSizeHint({.minimal = {0, 1}, .maximal = {100, 1}, .preferred = {0, 1}});
	layout.addWidget(&a);
	layout.addWidget(&b);
	layout.addWidget(&c);
	layout.setGeometry({.anchor = {0, 0}, .size = {101, 1}});
	EXPECT_EQ(a.geometry().width + b.geometry().width + c.geometry().width, 101u);
	EXPECT_EQ(b.geometry().x, static_cast<int>(a.geometry().width));
	EXPECT_EQ(c.geometry().x, static_cast<int>(a.geometry().width + b.geometry().width));
}
