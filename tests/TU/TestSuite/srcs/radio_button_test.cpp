#include <gtest/gtest.h>

#include <vector>

#include "ui/widget/radio_button.hpp"

TEST(RadioButtonTest, DetachedProgrammaticStateNotifiesOnlyOnChanges)
{
	spk::RadioButton button("Radio");
	std::vector<bool> states;
	auto contract = button.subscribeToState([&](bool checked) { states.push_back(checked); });
	button.setChecked(true);
	button.setChecked(true);
	button.setChecked(false);
	EXPECT_EQ(states, (std::vector<bool>{true, false}));
	EXPECT_FALSE(button.isChecked());
	EXPECT_EQ(button.group(), nullptr);
}

TEST(RadioButtonTest, GroupSelectionIsExclusiveAndProgrammaticUncheckHonorsPolicy)
{
	spk::RadioButtonGroup group("Group");
	auto &first = group.insert(0, 0, "First");
	auto &second = group.insert(1, 0, "Second");
	second.setChecked(true);
	EXPECT_FALSE(first.isChecked());
	EXPECT_TRUE(second.isChecked());
	second.setChecked(false);
	EXPECT_TRUE(second.isChecked());
	group.setAllowNoSelection(true);
	second.setChecked(false);
	EXPECT_FALSE(second.isChecked());
	EXPECT_EQ(group.selectedButton(), nullptr);
}

TEST(RadioButtonTest, TextStyleSpacingIndicatorAndGeometryAreExposed)
{
	spk::RadioButton button("Radio");
	button.applyStyle(spk::Widget::defaultStyle.get());
	button.setText(U"Unicode ✓");
	button.setSpacing(9);
	button.setIndicatorSize({24, 18});
	button.setGeometry({.anchor = {5, 6}, .size = {180, 30}});
	EXPECT_EQ(button.label().text(), U"Unicode ✓");
	EXPECT_EQ(button.indicator().preferredSize(), spk::Vector2(24, 18));
	EXPECT_EQ(button.indicator().geometry().size, spk::Vector2UInt(24, 18));
	EXPECT_EQ(button.label().parent(), &button);
	EXPECT_EQ(button.indicator().parent(), &button);
}
