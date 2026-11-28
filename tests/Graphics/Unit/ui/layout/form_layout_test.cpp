#include <gtest/gtest.h>

#include "ui/layout/form_layout.hpp"
#include "ui/widget.hpp"

TEST(FormLayoutTest, AddsRowsWithDefaultPoliciesAlignmentAndGeometry)
{
	spk::FormLayout form;
	spk::Widget label("label", nullptr), field("field", nullptr);
	label.setSizeHint({.minimal = {10, 10}, .maximal = {40, 30}, .preferred = {20, 20}});
	field.setSizeHint({.minimal = {20, 10}, .maximal = {100, 30}, .preferred = {50, 20}});
	const auto row = form.addRow(&label, &field);
	EXPECT_EQ(form.nbRow(), 1u);
	EXPECT_EQ(row.label->widget(), &label);
	EXPECT_EQ(row.field->widget(), &field);
	EXPECT_EQ(row.label->sizeSettings(), spk::Layout::SizeSettings(spk::Layout::SizePolicy::Fixed));
	EXPECT_EQ(row.field->horizontalAlignment(), spk::Alignment::Horizontal::Left);
	EXPECT_EQ(row.label->alignment(), (spk::Alignment{spk::Alignment::Horizontal::Right, spk::Alignment::Vertical::Center}));
	form.setGeometry({.anchor = {0, 0}, .size = {120, 40}});
	EXPECT_EQ(label.geometry().width, 20u);
	EXPECT_GE(field.geometry().width, 50u);
}

TEST(FormLayoutTest, NullLabelOrFieldIsRejectedWithoutAddingPartialRow)
{
	spk::FormLayout form;
	spk::Widget widget("widget", nullptr);
	EXPECT_THROW(form.addRow(nullptr, &widget), std::invalid_argument);
	EXPECT_THROW(form.addRow(&widget, nullptr), std::invalid_argument);
	EXPECT_EQ(form.nbRow(), 0u);
}

TEST(FormLayoutTest, RemovesMatchingRowsAndIgnoresStaleOrForeignDescriptors)
{
	spk::FormLayout form;
	spk::FormLayout foreign;
	spk::Widget l1("l1", nullptr), f1("f1", nullptr), l2("l2", nullptr), f2("f2", nullptr);
	const auto first = form.addRow(&l1, &f1);
	const auto second = form.addRow(&l2, &f2);
	const auto foreignRow = foreign.addRow(&l1, &f1);
	form.removeRow(foreignRow);
	EXPECT_EQ(form.nbRow(), 2u);
	form.removeRow(first);
	EXPECT_EQ(form.nbRow(), 1u);
	EXPECT_EQ(form.element(0, 0), second.label);
	form.removeRow(first);
	EXPECT_EQ(form.nbRow(), 1u);
}

TEST(FormLayoutTest, DuplicateWidgetUseIsCurrentlyPermitted)
{
	spk::FormLayout form;
	spk::Widget widget("shared", nullptr);
	EXPECT_NO_THROW((void)form.addRow(&widget, &widget));
	EXPECT_NO_THROW((void)form.addRow(&widget, &widget));
	EXPECT_EQ(form.nbRow(), 2u);
}
