#include <gtest/gtest.h>

#include "ui/layout/grid_layout.hpp"
#include "ui/widget.hpp"

namespace
{
	template <typename T>
	concept CanAddRow = requires(T &value) { value.addEmptyRow(); };
}

TEST(GridLayoutFixedRowsTest, KeepsRowsFixedAndGrowsColumnsAutomatically)
{
	using Grid = spk::GridLayoutFixedRows<3>;
	static_assert(!CanAddRow<Grid>);
	Grid grid;
	EXPECT_EQ(grid.rowCount(), 3u);
	EXPECT_EQ(grid.columnCount(), 0u);
	spk::Widget widget("widget", nullptr);
	grid.setWidget(4, 2, &widget);
	EXPECT_EQ(grid.columnCount(), 5u);
	EXPECT_EQ(grid.element(4, 2)->widget(), &widget);
	EXPECT_THROW(grid.setWidget(0, 3, &widget), std::out_of_range);
}

TEST(GridLayoutFixedRowsTest, ClearRestoresZeroColumnsAndPreservesRows)
{
	spk::GridLayoutFixedRows<2> grid;
	spk::Widget widget("widget", nullptr);
	grid.setWidget(1, 1, &widget);
	grid.clear();
	EXPECT_EQ(grid.columnCount(), 0u);
	EXPECT_EQ(grid.rowCount(), 2u);
}
