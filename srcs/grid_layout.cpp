#include "grid_layout.hpp"

#include <algorithm>

namespace
{
	template <typename THint>
	void _merge(THint &target, const THint &source)
	{
		target.minimal = std::max(target.minimal, source.minimal);
		target.preferred = std::max(target.preferred, source.preferred);
		target.maximal = std::max(target.maximal, source.maximal);
	}

	template <typename THints, typename TGetter>
	float _sum(const THints &hints, TGetter getter)
	{
		float result = 0.0f;
		for (const auto &hint : hints)
			result += getter(hint);
		return result;
	}
}

namespace spk
{
	GridLayout::GridLayout()
	{
		_updateSizeHint();
	}

	std::size_t GridLayout::_index(std::size_t row, std::size_t column) const noexcept
	{
		return row * _columnCount + column;
	}

	void GridLayout::_resizeGrid(std::size_t rows, std::size_t columns)
	{
		if (rows == _rowCount && columns == _columnCount)
			return;
		std::vector<std::unique_ptr<Element>> elements(rows * columns);
		const std::size_t rowsToCopy = std::min(rows, _rowCount);
		const std::size_t columnsToCopy = std::min(columns, _columnCount);
		for (std::size_t row = 0; row < rowsToCopy; ++row)
			for (std::size_t column = 0; column < columnsToCopy; ++column)
				elements[row * columns + column] = std::move(_elements[_index(row, column)]);
		_elements.swap(elements);
		_rowCount = rows;
		_columnCount = columns;
	}

	void GridLayout::_ensureSize(std::size_t rows, std::size_t columns)
	{
		_resizeGrid(std::max(rows, _rowCount), std::max(columns, _columnCount));
	}

	std::vector<Layout::AxisSizeHint> GridLayout::_columnHints() const
	{
		std::vector<AxisSizeHint> result(_columnCount);
		for (std::size_t row = 0; row < _rowCount; ++row)
			for (std::size_t column = 0; column < _columnCount; ++column)
				if (const auto &element = _elements[_index(row, column)]; element != nullptr)
					_merge(result[column], _horizontalHint(element->sizeHint()));
		return result;
	}

	std::vector<Layout::AxisSizeHint> GridLayout::_rowHints() const
	{
		std::vector<AxisSizeHint> result(_rowCount);
		for (std::size_t row = 0; row < _rowCount; ++row)
			for (std::size_t column = 0; column < _columnCount; ++column)
				if (const auto &element = _elements[_index(row, column)]; element != nullptr)
					_merge(result[row], _verticalHint(element->sizeHint()));
		return result;
	}

	float GridLayout::_horizontalPadding() const noexcept
	{
		return (_columnCount < 2) ? 0.0f : static_cast<float>(_columnCount - 1) * static_cast<float>(_elementPadding.x);
	}

	float GridLayout::_verticalPadding() const noexcept
	{
		return (_rowCount < 2) ? 0.0f : static_cast<float>(_rowCount - 1) * static_cast<float>(_elementPadding.y);
	}

	ResizeableTrait::SizeHint GridLayout::_computedSizeHint() const
	{
		const auto columns = _columnHints();
		const auto rows = _rowHints();
		SizeHint result;
		result.minimal = {_sum(columns, [](const auto &hint) { return hint.minimal; }) + _horizontalPadding(), _sum(rows, [](const auto &hint) { return hint.minimal; }) + _verticalPadding()};
		result.preferred = {_sum(columns, [](const auto &hint) { return hint.preferred; }) + _horizontalPadding(), _sum(rows, [](const auto &hint) { return hint.preferred; }) + _verticalPadding()};
		result.maximal = {_sum(columns, [](const auto &hint) { return hint.maximal; }) + _horizontalPadding(), _sum(rows, [](const auto &hint) { return hint.maximal; }) + _verticalPadding()};
		return result;
	}

	void GridLayout::_updateSizeHint()
	{
		_setComputedSizeHint(_computedSizeHint());
	}

	void GridLayout::_applyGeometry(const Rect2D &geometry)
	{
		const float availableWidth = std::max(0.0f, static_cast<float>(geometry.width) - _horizontalPadding());
		const float availableHeight = std::max(0.0f, static_cast<float>(geometry.height) - _verticalPadding());
		const std::vector<float> widths = _resolveAxis(_columnHints(), availableWidth);
		const std::vector<float> heights = _resolveAxis(_rowHints(), availableHeight);
		int32_t y = geometry.y;
		for (std::size_t row = 0; row < _rowCount; ++row)
		{
			int32_t x = geometry.x;
			const uint32_t height = _dimension(heights[row]);
			for (std::size_t column = 0; column < _columnCount; ++column)
			{
				const uint32_t width = _dimension(widths[column]);
				if (auto &element = _elements[_index(row, column)]; element != nullptr)
					element->setGeometry(_rect(x, y, width, height));
				x += static_cast<int32_t>(width + _elementPadding.x);
			}
			y += static_cast<int32_t>(height + _elementPadding.y);
		}
	}

	void GridLayout::clear()
	{
		_elements.clear();
		_rowCount = 0;
		_columnCount = 0;
		updateSizeHint();
	}

	std::size_t GridLayout::rowCount() const noexcept
	{
		return _rowCount;
	}

	std::size_t GridLayout::columnCount() const noexcept
	{
		return _columnCount;
	}

	void GridLayout::addEmptyRow()
	{
		_resizeGrid(_rowCount + 1, std::max<std::size_t>(1, _columnCount));
		updateSizeHint();
	}

	void GridLayout::addEmptyColumn()
	{
		_resizeGrid(std::max<std::size_t>(1, _rowCount), _columnCount + 1);
		updateSizeHint();
	}

	void GridLayout::removeRow(std::size_t row)
	{
		if (row >= _rowCount)
			throw std::out_of_range("GridLayout::removeRow: invalid row index");
		std::vector<std::unique_ptr<Element>> elements((_rowCount - 1) * _columnCount);
		std::size_t targetRow = 0;
		for (std::size_t sourceRow = 0; sourceRow < _rowCount; ++sourceRow)
		{
			if (sourceRow == row)
				continue;
			for (std::size_t column = 0; column < _columnCount; ++column)
				elements[targetRow * _columnCount + column] = std::move(_elements[_index(sourceRow, column)]);
			++targetRow;
		}
		_elements.swap(elements);
		--_rowCount;
		updateSizeHint();
	}

	void GridLayout::removeColumn(std::size_t column)
	{
		if (column >= _columnCount)
			throw std::out_of_range("GridLayout::removeColumn: invalid column index");
		std::vector<std::unique_ptr<Element>> elements(_rowCount * (_columnCount - 1));
		for (std::size_t row = 0; row < _rowCount; ++row)
			for (std::size_t source = 0, target = 0; source < _columnCount; ++source)
				if (source != column)
					elements[row * (_columnCount - 1) + target++] = std::move(_elements[_index(row, source)]);
		_elements.swap(elements);
		--_columnCount;
		updateSizeHint();
	}

	Layout::Element *GridLayout::setWidget(std::size_t column, std::size_t row, Widget *widget, SizePolicy sizePolicy)
	{
		_ensureSize(row + 1, column + 1);
		auto element = _createElement(widget, sizePolicy);
		Element *result = element.get();
		_elements[_index(row, column)] = std::move(element);
		updateSizeHint();
		return result;
	}

	Layout::Element *GridLayout::setLayout(std::size_t column, std::size_t row, Layout *layout, SizePolicy sizePolicy)
	{
		_ensureSize(row + 1, column + 1);
		auto element = _createElement(layout, sizePolicy);
		Element *result = element.get();
		_elements[_index(row, column)] = std::move(element);
		updateSizeHint();
		return result;
	}

	void GridLayout::clearCell(std::size_t column, std::size_t row)
	{
		if (row >= _rowCount || column >= _columnCount)
			throw std::out_of_range("GridLayout::clearCell: invalid cell index");
		_elements[_index(row, column)].reset();
		updateSizeHint();
	}

	Layout::Element *GridLayout::element(std::size_t column, std::size_t row) noexcept
	{
		if (row >= _rowCount || column >= _columnCount)
			return nullptr;
		return _elements[_index(row, column)].get();
	}

	const Layout::Element *GridLayout::element(std::size_t column, std::size_t row) const noexcept
	{
		if (row >= _rowCount || column >= _columnCount)
			return nullptr;
		return _elements[_index(row, column)].get();
	}
}
