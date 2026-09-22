#include <gtest/gtest.h>

#include "ui/layout/grid_layout.hpp"
#include "ui/layout/linear_layout.hpp"
#include "ui/widget.hpp"

TEST(GridLayoutTest, SparseGrowthOverwriteAndCellAccessAreObservable)
{
	spk::GridLayout grid;
	spk::Widget first("first", nullptr), replacement("replacement", nullptr);
	auto *firstElement = grid.setWidget(2, 1, &first);
	EXPECT_EQ(grid.columnCount(), 3u);
	EXPECT_EQ(grid.rowCount(), 2u);
	EXPECT_EQ(grid.element(2, 1), firstElement);
	EXPECT_EQ(grid.element(0, 0), nullptr);
	auto *replacementElement = grid.setWidget(2, 1, &replacement);
	EXPECT_EQ(grid.element(2, 1), replacementElement);
	EXPECT_EQ(replacementElement->widget(), &replacement);
	EXPECT_EQ(std::as_const(grid).element(99, 99), nullptr);
}

TEST(GridLayoutTest, RowsColumnsAndCellsCanBeAddedRemovedAndCleared)
{
	spk::GridLayout grid;
	grid.addEmptyRow();
	grid.addEmptyColumn();
	grid.addEmptyRow();
	grid.addEmptyColumn();
	EXPECT_EQ(grid.rowCount(), 2u);
	EXPECT_EQ(grid.columnCount(), 3u);
	spk::Widget widget("widget", nullptr);
	grid.setWidget(2, 1, &widget);
	grid.removeColumn(1);
	EXPECT_EQ(grid.columnCount(), 2u);
	EXPECT_EQ(grid.element(1, 1)->widget(), &widget);
	grid.removeRow(0);
	EXPECT_EQ(grid.rowCount(), 1u);
	grid.clearCell(1, 0);
	EXPECT_EQ(grid.element(1, 0), nullptr);
	grid.clear();
	EXPECT_EQ(grid.rowCount(), 0u);
	EXPECT_EQ(grid.columnCount(), 0u);
}

TEST(GridLayoutTest, InvalidRemovalAndClearIndicesThrow)
{
	spk::GridLayout grid;
	EXPECT_THROW(grid.removeRow(0), std::out_of_range);
	EXPECT_THROW(grid.removeColumn(0), std::out_of_range);
	EXPECT_THROW(grid.clearCell(0, 0), std::out_of_range);
	grid.addEmptyRow();
	EXPECT_THROW(grid.removeRow(1), std::out_of_range);
	EXPECT_THROW(grid.clearCell(1, 0), std::out_of_range);
}

TEST(GridLayoutTest, ComputesHintsPaddingAndGeometryForWidgetAndNestedLayout)
{
	spk::GridLayout grid;
	spk::Widget widget("widget", nullptr);
	widget.setSizeHint({.minimal = {10, 20}, .maximal = {30, 40}, .preferred = {20, 30}});
	spk::HorizontalLayout nested;
	spk::Widget nestedWidget("nested", nullptr);
	nestedWidget.setSizeHint({.minimal = {5, 6}, .maximal = {50, 60}, .preferred = {25, 35}});
	nested.addWidget(&nestedWidget);
	grid.setWidget(0, 0, &widget);
	grid.setLayout(1, 0, &nested);
	grid.setElementPadding({4, 7});
	EXPECT_EQ(grid.preferredSize(), spk::Vector2(49, 35));
	grid.setGeometry({.anchor = {10, 20}, .size = {104, 60}});
	EXPECT_EQ(widget.geometry().x, 10);
	EXPECT_EQ(nestedWidget.geometry().y, 20);
}
