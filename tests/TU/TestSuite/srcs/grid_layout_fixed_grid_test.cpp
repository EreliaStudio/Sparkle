#include <gtest/gtest.h>

#include "ui/layout/grid_layout.hpp"
#include "ui/widget.hpp"

namespace
{
	template <typename T>
	concept CanResizeRows = requires(T &value) { value.addEmptyRow(); };
	template <typename T>
	concept CanResizeColumns = requires(T &value) { value.addEmptyColumn(); };
}

TEST(GridLayoutFixedGridTest, DimensionsStayImmutableAndEveryBoundaryCellIsUsable)
{
	using Grid = spk::GridLayoutFixedGrid<3, 2>;
	static_assert(!CanResizeRows<Grid>);
	static_assert(!CanResizeColumns<Grid>);
	Grid grid;
	spk::Widget first("first", nullptr), last("last", nullptr);
	grid.setWidget(0, 0, &first);
	grid.setWidget(2, 1, &last);
	EXPECT_EQ(grid.columnCount(), 3u);
	EXPECT_EQ(grid.rowCount(), 2u);
	EXPECT_EQ(grid.element(0, 0)->widget(), &first);
	EXPECT_EQ(grid.element(2, 1)->widget(), &last);
	EXPECT_THROW(grid.setWidget(3, 0, &first), std::out_of_range);
	EXPECT_THROW(grid.setWidget(0, 2, &first), std::out_of_range);
}

TEST(GridLayoutFixedGridTest, ClearRemovesCellsWithoutChangingDimensions)
{
	spk::GridLayoutFixedGrid<2, 2> grid;
	spk::Widget widget("widget", nullptr);
	grid.setWidget(1, 1, &widget);
	grid.clear();
	EXPECT_EQ(grid.columnCount(), 2u);
	EXPECT_EQ(grid.rowCount(), 2u);
	EXPECT_EQ(grid.element(1, 1), nullptr);
}
