#include <gtest/gtest.h>

#include "ui/layout/grid_layout.hpp"
#include "ui/widget.hpp"

namespace
{
	template <typename T>
	concept CanAddColumn = requires(T &value) { value.addEmptyColumn(); };
}

TEST(GridLayoutFixedColumnsTest, KeepsColumnsFixedAndGrowsRowsAutomatically)
{
	using Grid = spk::GridLayoutFixedColumns<3>;
	static_assert(!CanAddColumn<Grid>);
	Grid grid;
	EXPECT_EQ(grid.columnCount(), 3u);
	EXPECT_EQ(grid.rowCount(), 0u);
	spk::Widget widget("widget", nullptr);
	grid.setWidget(2, 4, &widget);
	EXPECT_EQ(grid.rowCount(), 5u);
	EXPECT_EQ(grid.element(2, 4)->widget(), &widget);
	EXPECT_THROW(grid.setWidget(3, 0, &widget), std::out_of_range);
}

TEST(GridLayoutFixedColumnsTest, ClearRestoresZeroRowsAndPreservesColumns)
{
	spk::GridLayoutFixedColumns<2> grid;
	spk::Widget widget("widget", nullptr);
	grid.setWidget(1, 1, &widget);
	grid.clear();
	EXPECT_EQ(grid.rowCount(), 0u);
	EXPECT_EQ(grid.columnCount(), 2u);
}
