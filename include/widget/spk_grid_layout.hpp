#pragma once

#include <numeric>

#include "widget/spk_layout.hpp"

namespace spk
{
	template <size_t NbColumns, size_t NbRows>
	class GridLayout : public Layout
	{
	private:
		using Layout::addWidget;
		using Layout::removeWidget;

	public:
		GridLayout() :
			Layout()
		{
			resizeElements(NbColumns * NbRows);
		}

		spk::SafePointer<Element> setWidget(const size_t &p_col,
											const size_t &p_row,
											spk::SafePointer<spk::Widget> p_widget,
											const SizePolicy &p_sizePolicy = SizePolicy::Extend)
		{
			if (p_col >= NbColumns || p_row >= NbRows)
			{
				throw std::invalid_argument("GridLayout::setWidget: Invalid column or row index");
			}

			size_t index = p_row * NbColumns + p_col;

			if (_elements[index] == nullptr)
			{
				_elements[index] = std::unique_ptr<Element>(new Element(p_widget, p_sizePolicy, {1, 1}));
			}
			else
			{
				*_elements[index] = Element(p_widget, p_sizePolicy, {1, 1});
			}

			return (_elements[index].get());
		}

		void setGeometry(const spk::Geometry2D &p_geometry) override
		{
			std::array<int, NbColumns> colWidth{};
			std::array<int, NbRows> rowHeight{};

			for (size_t r = 0; r < NbRows; ++r)
			{
				for (size_t c = 0; c < NbColumns; ++c)
				{
					Element *elt = _elements[r * NbColumns + c].get();
					if (elt == nullptr || (elt->widget() == nullptr && elt->layout() == nullptr))
					{
						continue;
					}

					const spk::Vector2Int req = elt->size();

					switch (elt->sizePolicy())
					{
					case SizePolicy::Fixed:
					{
						colWidth[c] = std::max(colWidth[c], req.x);
						rowHeight[r] = std::max(rowHeight[r], req.y);
						break;
					}
					case SizePolicy::Minimum:
					case SizePolicy::VerticalExtend:
					{
						spk::Vector2Int minSize = elt->minimalSize();
						colWidth[c] = std::max(colWidth[c], minSize.x);
						rowHeight[r] = std::max(rowHeight[r], minSize.y);
						break;
					}
					case SizePolicy::Maximum:
					{
						spk::Vector2Int maxSize = elt->maximalSize();
						colWidth[c] = std::min(colWidth[c], maxSize.x);
						rowHeight[r] = std::min(rowHeight[r], maxSize.y);
						break;
					}
					case SizePolicy::Extend:
					case SizePolicy::HorizontalExtend:
					{
						break;
					}
					}
				}
			}

			int minTotalW = std::accumulate(colWidth.begin(), colWidth.end(), 0);
			int minTotalH = std::accumulate(rowHeight.begin(), rowHeight.end(), 0);

			minTotalW += static_cast<int>((NbColumns - 1) * _elementPadding.x);
			minTotalH += static_cast<int>((NbRows - 1) * _elementPadding.y);

			int extraW = p_geometry.size.x > minTotalW ? p_geometry.size.x - minTotalW : 0;
			int extraH = p_geometry.size.y > minTotalH ? p_geometry.size.y - minTotalH : 0;

			std::array<bool, NbColumns> colIsExt{};
			int nbExtCols = 0;

			for (size_t c = 0; c < NbColumns; ++c)
			{
				for (size_t r = 0; r < NbRows; ++r)
				{
					Element *elt = _elements[r * NbColumns + c].get();
					if (elt && elt->sizePolicy() == SizePolicy::Extend || elt->sizePolicy() == SizePolicy::HorizontalExtend)
					{
						colIsExt[c] = true;
						break;
					}
				}
				if (colIsExt[c])
				{
					++nbExtCols;
				}
			}
			if (nbExtCols == 0)
			{
				nbExtCols = NbColumns;
				colIsExt.fill(true);
			}

			int wShare = extraW / nbExtCols;
			int wRemain = extraW % nbExtCols;
			for (size_t c = 0; c < NbColumns; ++c)
			{
				if (colIsExt[c])
				{
					colWidth[c] += wShare + (wRemain-- > 0 ? 1 : 0);
				}
			}

			std::array<bool, NbRows> rowIsExt{};
			int nbExtRows = 0;

			for (size_t r = 0; r < NbRows; ++r)
			{
				for (size_t c = 0; c < NbColumns; ++c)
				{
					Element *elt = _elements[r * NbColumns + c].get();
					if (elt && (elt->sizePolicy() == SizePolicy::Extend || elt->sizePolicy() == SizePolicy::VerticalExtend))
					{
						rowIsExt[r] = true;
						break;
					}
				}
				if (rowIsExt[r])
				{
					++nbExtRows;
				}
			}
			if (nbExtRows == 0)
			{
				nbExtRows = NbRows;
				rowIsExt.fill(true);
			}

			int hShare = extraH / nbExtRows;
			int hRemain = extraH % nbExtRows;
			for (size_t r = 0; r < NbRows; ++r)
			{
				if (rowIsExt[r])
				{
					rowHeight[r] += hShare + (hRemain-- > 0 ? 1 : 0);
				}
			}

			int y = p_geometry.anchor.y;
			for (size_t r = 0; r < NbRows; ++r)
			{
				int x = p_geometry.anchor.x;
				for (size_t c = 0; c < NbColumns; ++c)
				{
					Element *elt = _elements[r * NbColumns + c].get();
					if (elt && (elt->widget() != nullptr || elt->layout() != nullptr))
					{
						spk::Geometry2D cellGeom({x, y}, {colWidth[c], rowHeight[r]});
						elt->setGeometry(cellGeom);
					}
					x += colWidth[c] + static_cast<int>(_elementPadding.x);
				}
				y += rowHeight[r] + static_cast<int>(_elementPadding.y);
			}
		}

		spk::Vector2UInt minimalSize() const
		{
			std::array<uint32_t, NbColumns> colWidth{};
			std::array<uint32_t, NbRows> rowHeight{};

			for (size_t r = 0; r < NbRows; ++r)
			{
				for (size_t c = 0; c < NbColumns; ++c)
				{
					Element *elt = _elements[r * NbColumns + c].get();
					if (!elt || (elt->widget() == nullptr && elt->layout() == nullptr))
					{
						continue;
					}

					spk::Vector2UInt minSize = elt->minimalSize();

					colWidth[c] = std::max(colWidth[c], minSize.x);
					rowHeight[r] = std::max(rowHeight[r], minSize.y);
				}
			}

			uint32_t totalW = std::accumulate(colWidth.begin(), colWidth.end(), 0u);
			uint32_t totalH = std::accumulate(rowHeight.begin(), rowHeight.end(), 0u);

			totalW += static_cast<uint32_t>((NbColumns > 0 ? NbColumns - 1 : 0) * _elementPadding.x);
			totalH += static_cast<uint32_t>((NbRows > 0 ? NbRows - 1 : 0) * _elementPadding.y);

			return {totalW, totalH};
		}
	};
}
