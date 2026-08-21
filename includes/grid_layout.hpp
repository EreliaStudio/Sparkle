#pragma once

#include <cstddef>
#include <stdexcept>
#include <vector>

#include "layout.hpp"

namespace spk
{
	class GridLayout : public Layout
	{
	private:
		std::size_t _rowCount = 0;
		std::size_t _columnCount = 0;

		[[nodiscard]] std::vector<AxisSizeHint> _columnHints() const;
		[[nodiscard]] std::vector<AxisSizeHint> _rowHints() const;
		[[nodiscard]] SizeHint _computedSizeHint() const;
		[[nodiscard]] float _horizontalPadding() const noexcept;
		[[nodiscard]] float _verticalPadding() const noexcept;

	protected:
		[[nodiscard]] std::size_t _index(std::size_t row, std::size_t column) const noexcept;
		void _resizeGrid(std::size_t rows, std::size_t columns);
		void _ensureSize(std::size_t rows, std::size_t columns);
		void _updateSizeHint() override;
		void _applyGeometry(const Rect2D &geometry) override;

	public:
		GridLayout();

		void clear() override;
		[[nodiscard]] std::size_t rowCount() const noexcept;
		[[nodiscard]] std::size_t columnCount() const noexcept;

		void addEmptyRow();
		void addEmptyColumn();
		void removeRow(std::size_t row);
		void removeColumn(std::size_t column);

		Element *setWidget(std::size_t column, std::size_t row, Widget *widget, SizePolicy sizePolicy = SizePolicy::Extend);
		Element *setLayout(std::size_t column, std::size_t row, Layout *layout, SizePolicy sizePolicy = SizePolicy::Extend);
		void clearCell(std::size_t column, std::size_t row);

		[[nodiscard]] Element *element(std::size_t column, std::size_t row) noexcept;
		[[nodiscard]] const Element *element(std::size_t column, std::size_t row) const noexcept;
	};

	template <std::size_t NbColumns>
	class GridLayoutFixedColumns : public GridLayout
	{
	public:
		GridLayoutFixedColumns()
		{
			_resizeGrid(0, NbColumns);
			updateSizeHint();
		}

		[[nodiscard]] static constexpr std::size_t columnCount() noexcept
		{
			return NbColumns;
		}

		void clear() override
		{
			_resizeGrid(0, NbColumns);
			updateSizeHint();
		}

		void addEmptyColumn() = delete;
		void removeColumn(std::size_t) = delete;

		Element *setWidget(std::size_t column, std::size_t row, Widget *widget, SizePolicy sizePolicy = SizePolicy::Extend)
		{
			_validateColumn(column);
			_ensureSize(row + 1, NbColumns);
			return GridLayout::setWidget(column, row, widget, sizePolicy);
		}

		Element *setLayout(std::size_t column, std::size_t row, Layout *layout, SizePolicy sizePolicy = SizePolicy::Extend)
		{
			_validateColumn(column);
			_ensureSize(row + 1, NbColumns);
			return GridLayout::setLayout(column, row, layout, sizePolicy);
		}

	private:
		static void _validateColumn(std::size_t column)
		{
			if (column >= NbColumns)
				throw std::out_of_range("GridLayoutFixedColumns: invalid column index");
		}
	};

	template <std::size_t NbRows>
	class GridLayoutFixedRows : public GridLayout
	{
	public:
		GridLayoutFixedRows()
		{
			_resizeGrid(NbRows, 0);
			updateSizeHint();
		}

		[[nodiscard]] static constexpr std::size_t rowCount() noexcept
		{
			return NbRows;
		}

		void clear() override
		{
			_resizeGrid(NbRows, 0);
			updateSizeHint();
		}

		void addEmptyRow() = delete;
		void removeRow(std::size_t) = delete;

		Element *setWidget(std::size_t column, std::size_t row, Widget *widget, SizePolicy sizePolicy = SizePolicy::Extend)
		{
			_validateRow(row);
			_ensureSize(NbRows, column + 1);
			return GridLayout::setWidget(column, row, widget, sizePolicy);
		}

		Element *setLayout(std::size_t column, std::size_t row, Layout *layout, SizePolicy sizePolicy = SizePolicy::Extend)
		{
			_validateRow(row);
			_ensureSize(NbRows, column + 1);
			return GridLayout::setLayout(column, row, layout, sizePolicy);
		}

	private:
		static void _validateRow(std::size_t row)
		{
			if (row >= NbRows)
				throw std::out_of_range("GridLayoutFixedRows: invalid row index");
		}
	};

	template <std::size_t NbColumns, std::size_t NbRows>
	class GridLayoutFixedGrid : public GridLayout
	{
	public:
		GridLayoutFixedGrid()
		{
			_resizeGrid(NbRows, NbColumns);
			updateSizeHint();
		}

		[[nodiscard]] static constexpr std::size_t columnCount() noexcept
		{
			return NbColumns;
		}

		[[nodiscard]] static constexpr std::size_t rowCount() noexcept
		{
			return NbRows;
		}

		void clear() override
		{
			for (auto &element : _elements)
				element.reset();
			updateSizeHint();
		}

		void addEmptyRow() = delete;
		void addEmptyColumn() = delete;
		void removeRow(std::size_t) = delete;
		void removeColumn(std::size_t) = delete;

		Element *setWidget(std::size_t column, std::size_t row, Widget *widget, SizePolicy sizePolicy = SizePolicy::Extend)
		{
			_validateCell(column, row);
			return GridLayout::setWidget(column, row, widget, sizePolicy);
		}

		Element *setLayout(std::size_t column, std::size_t row, Layout *layout, SizePolicy sizePolicy = SizePolicy::Extend)
		{
			_validateCell(column, row);
			return GridLayout::setLayout(column, row, layout, sizePolicy);
		}

	private:
		static void _validateCell(std::size_t column, std::size_t row)
		{
			if (column >= NbColumns || row >= NbRows)
				throw std::out_of_range("GridLayoutFixedGrid: invalid cell index");
		}
	};
}
