#include <gtest/gtest.h>

#include <optional>
#include <stdexcept>
#include <vector>

#include "ui/widget/radio_button.hpp"

TEST(RadioButtonGroupTest, SparseInsertionSelectionAndAccessPreserveInsertionOrder)
{
	spk::RadioButtonGroup group("Group");
	spk::RadioButton &first = group.insert(3, 1, "First");
	spk::RadioButton &second = group.insert(0, 4, "Second");
	spk::RadioButton &third = group.insert(2, 0, "Third");

	EXPECT_EQ(group.size(), 3u);
	EXPECT_EQ(group.columnCount(), 4u);
	EXPECT_EQ(group.rowCount(), 5u);
	EXPECT_EQ(group.button(3, 1), &first);
	EXPECT_EQ(group.button(0, 4), &second);
	EXPECT_EQ(group.button(1, 1), nullptr);
	EXPECT_EQ(&group.buttonAt(0), &first);
	EXPECT_EQ(&group.buttonAt(1), &second);
	EXPECT_EQ(&group.buttonAt(2), &third);
	EXPECT_EQ(group.selectedButton(), &first);
	EXPECT_EQ(group.selectedCell(), (std::optional<spk::RadioButtonGroup::Cell>{{3, 1}}));

	std::vector<std::optional<spk::RadioButtonGroup::Selection>> editions;
	auto contract = group.subscribeToSelection([&](auto selection) { editions.push_back(selection); });
	group.select(0, 4);
	group.select(second);

	ASSERT_EQ(editions.size(), 1u);
	ASSERT_TRUE(editions.front().has_value());
	EXPECT_EQ(editions.front()->button, &second);
	EXPECT_EQ(editions.front()->cell, (spk::RadioButtonGroup::Cell{0, 4}));
	EXPECT_FALSE(first.isChecked());
	EXPECT_TRUE(second.isChecked());
	EXPECT_FALSE(third.isChecked());
}

TEST(RadioButtonGroupTest, MandatoryAndOptionalSelectionPoliciesHandleRemovalAndClear)
{
	spk::RadioButtonGroup group("Group");
	spk::RadioButton &first = group.insert(0, 0, "First");
	spk::RadioButton &second = group.insert(1, 0, "Second");
	group.select(second);

	group.clearSelection();
	EXPECT_EQ(group.selectedButton(), &second);
	group.erase(second);
	EXPECT_EQ(group.selectedButton(), &first);
	EXPECT_TRUE(first.isChecked());

	group.setAllowNoSelection(true);
	EXPECT_TRUE(group.allowsNoSelection());
	group.clearSelection();
	EXPECT_EQ(group.selectedButton(), nullptr);
	EXPECT_FALSE(first.isChecked());
	group.erase(0, 0);
	EXPECT_EQ(group.size(), 0u);
	EXPECT_FALSE(group.selection().has_value());

	spk::RadioButton &replacement = group.insert(5, 5, "Replacement");
	EXPECT_EQ(group.selectedButton(), nullptr);
	group.setAllowNoSelection(false);
	EXPECT_EQ(group.selectedButton(), &replacement);
	group.clear();
	EXPECT_EQ(group.size(), 0u);
	EXPECT_EQ(group.selectedButton(), nullptr);
}

TEST(RadioButtonGroupTest, GeometryAndPresentationSettingsPropagateToExistingAndFutureButtons)
{
	spk::RadioButtonGroup group("Group");
	spk::RadioButton &existing = group.insert(0, 0, "Existing");
	group.setSpacing(11);
	group.setIndicatorSize({23, 17});
	group.setElementPadding({7, 9});
	const auto &style = spk::Widget::defaultStyle.get();
	ASSERT_NE(style.font, nullptr);
	ASSERT_NE(style.iconset, nullptr);
	group.setFont(style.font.get());
	group.setIconset(style.iconset.get());
	group.setSpriteIDs(4, 5);
	spk::RadioButton &future = group.insert(1, 1, "Future");

	EXPECT_EQ(group.spacing(), 11u);
	EXPECT_EQ(group.indicatorSize(), spk::Vector2UInt(23, 17));
	EXPECT_EQ(group.elementPadding(), spk::Vector2UInt(7, 9));
	EXPECT_EQ(existing.indicator().preferredSize(), spk::Vector2(23, 17));
	EXPECT_EQ(future.indicator().preferredSize(), spk::Vector2(23, 17));
	EXPECT_EQ(existing.label().text(), U"Existing");
	EXPECT_EQ(future.label().text(), U"Future");
	EXPECT_EQ(existing.label().font(), style.font.get());
	EXPECT_EQ(future.label().font(), style.font.get());
	EXPECT_EQ(existing.indicator().uncheckedButton().iconset(), style.iconset.get());
	EXPECT_EQ(future.indicator().checkedButton().iconset(), style.iconset.get());
	EXPECT_EQ(existing.indicator().uncheckedSpriteID(), 4u);
	EXPECT_EQ(future.indicator().checkedSpriteID(), 5u);

	group.setGeometry({.anchor = {10, 20}, .size = {240, 100}});
	EXPECT_TRUE(existing.hasParent());
	EXPECT_EQ(existing.parent(), &group);
	EXPECT_TRUE(future.hasParent());
	EXPECT_EQ(future.parent(), &group);
}

TEST(RadioButtonGroupTest, InvalidSelectionInsertionResourcesAndIndicesThrowWithoutMutation)
{
	spk::RadioButtonGroup group("Group");
	spk::RadioButton &member = group.insert(2, 3, "Member");
	spk::RadioButton foreign("Foreign");

	EXPECT_THROW((void)group.insert(2, 3, "Duplicate"), std::invalid_argument);
	EXPECT_THROW(group.select(9, 9), std::invalid_argument);
	EXPECT_THROW(group.select(foreign), std::invalid_argument);
	EXPECT_THROW((void)group.buttonAt(1), std::out_of_range);
	EXPECT_THROW(group.setFont(nullptr), std::invalid_argument);
	EXPECT_THROW(group.setIconset(nullptr), std::invalid_argument);

	EXPECT_EQ(group.size(), 1u);
	EXPECT_EQ(group.button(2, 3), &member);
	EXPECT_EQ(group.selectedButton(), &member);
	group.erase(foreign);
	group.erase(9, 9);
	EXPECT_EQ(group.size(), 1u);
}
