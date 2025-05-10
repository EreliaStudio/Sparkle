#include "widget/spk_form_layout.hpp"

#include <numeric>

namespace spk
{
	FormLayout::FormElement FormLayout::addRow(spk::SafePointer<spk::Widget> p_labelWidget,
					spk::SafePointer<spk::Widget> p_fieldWidget,
					const SizePolicy &p_labelPolicy,
					const SizePolicy &p_fieldPolicy)
	{
		return FormElement{
			addWidget(p_labelWidget, p_labelPolicy),
			addWidget(p_fieldWidget, p_fieldPolicy)};
	}

	void FormLayout::removeRow(const FormElement& p_row)
	{
		removeWidget(p_row.labelElement);
		removeWidget(p_row.fieldElement);
	}

	size_t FormLayout::nbRow() const { return _elements.size() / 2; }

	void FormLayout::setGeometry(const spk::Geometry2D &p_geometry)
	{
		const std::size_t rowCountValue = nbRow();
		if (rowCountValue == 0)
		{
			return;
		}

		uint32_t labelColumnWidth = 0;
		uint32_t fieldColumnWidth = 0;
		std::vector<int> rowHeights(rowCountValue, 0);
		std::vector<bool> rowIsExpandable(rowCountValue, false);
		bool labelColumnIsExpandable = false;
		bool fieldColumnIsExpandable = false;

		auto updateColumn = [&](Element *p_element,
								uint32_t &p_columnWidth,
								bool     &p_columnIsExpandable)
		{
			if (p_element == nullptr || p_element->widget() == nullptr)
			{
				return;
			}

			const spk::Vector2UInt requestedSize = p_element->size();

			switch (p_element->sizePolicy())
			{
			case SizePolicy::Fixed:
			{
				p_columnWidth = std::max(p_columnWidth, requestedSize.x);
				break;
			}
			case SizePolicy::Minimum:
			case SizePolicy::VerticalExtend:
			{
				p_columnWidth = std::max(p_columnWidth, p_element->widget()->minimalSize().x);
				break;
			}
			case SizePolicy::Maximum:
			{
				p_columnWidth = std::min(p_columnWidth, p_element->widget()->maximalSize().x);
				break;
			}
			case SizePolicy::Extend:
			case SizePolicy::HorizontalExtend:
			{
				p_columnIsExpandable = true;
				break;
			}
			}
		};

		auto heightForElement = [](Element *p_element) -> int
		{
			if (p_element == nullptr || p_element->widget() == nullptr)
			{
				return 0;
			}

			switch (p_element->sizePolicy())
			{
			case SizePolicy::Fixed:
			{
				return p_element->size().y;
			}
			case SizePolicy::Minimum:
			case SizePolicy::HorizontalExtend:
			{
				return p_element->widget()->minimalSize().y;
			}
			case SizePolicy::Maximum:
			{	
				return p_element->widget()->maximalSize().y;
			}
			case SizePolicy::Extend:
			case SizePolicy::VerticalExtend:
			{
				return p_element->widget()->minimalSize().y;
			}
			}
			return 0;
		};

		for (std::size_t row = 0; row < rowCountValue; ++row)
		{
			Element *labelElement = _elements[2 * row].get();
			Element *fieldElement = _elements[2 * row + 1].get();

			updateColumn(labelElement, labelColumnWidth, labelColumnIsExpandable);
			updateColumn(fieldElement, fieldColumnWidth, fieldColumnIsExpandable);

			rowHeights[row] = std::max(heightForElement(labelElement), heightForElement(fieldElement));

			if ((labelElement && (labelElement->sizePolicy() == SizePolicy::Extend ||
                                 labelElement->sizePolicy() == SizePolicy::VerticalExtend)) ||
				(fieldElement && (fieldElement->sizePolicy() == SizePolicy::Extend ||
                                 fieldElement->sizePolicy() == SizePolicy::VerticalExtend)))
			{
				rowIsExpandable[row] = true;
			}
		}

		const uint32_t minimumTotalWidth  = labelColumnWidth + fieldColumnWidth + _elementPadding.x;
		const uint32_t minimumTotalHeight = static_cast<uint32_t>(std::accumulate(rowHeights.begin(), rowHeights.end(), 0)) +
											static_cast<uint32_t>((rowCountValue > 0U ? rowCountValue - 1U : 0U) * _elementPadding.y);

		uint32_t extraWidth  = p_geometry.size.x > minimumTotalWidth ? p_geometry.size.x - minimumTotalWidth  : 0U;
		uint32_t extraHeight = p_geometry.size.y > minimumTotalHeight ? p_geometry.size.y - minimumTotalHeight : 0U;

		uint32_t expandableColumnCount = (labelColumnIsExpandable ? 1 : 0) + (fieldColumnIsExpandable ? 1 : 0);

		if (expandableColumnCount == 0)
		{
			labelColumnIsExpandable = true;
			fieldColumnIsExpandable = true;
			expandableColumnCount = 2;
		}

		uint32_t widthShare = extraWidth / expandableColumnCount;
		uint32_t widthRemainder = extraWidth % expandableColumnCount;

		if (labelColumnIsExpandable == true)
		{
			labelColumnWidth += widthShare + ((widthRemainder-- > 0) ? 1 : 0);
		}

		if (fieldColumnIsExpandable == true)
		{
			fieldColumnWidth += widthShare + ((widthRemainder-- > 0) ? 1 : 0);
		}

		uint32_t expandableRowCount = static_cast<uint32_t>(std::count(rowIsExpandable.begin(), rowIsExpandable.end(), true));

		if (expandableRowCount == 0)
		{
			std::fill(rowIsExpandable.begin(), rowIsExpandable.end(), true);
			expandableRowCount = static_cast<uint32_t>(rowCountValue);
		}

		uint32_t heightShare     = extraHeight / expandableRowCount;
		uint32_t heightRemainder = extraHeight % expandableRowCount;

		for (std::size_t row = 0; row < rowCountValue; ++row)
		{
			if (rowIsExpandable[row] == true)
			{
				rowHeights[row] += heightShare + ((heightRemainder-- > 0) ? 1 : 0);
			}
		}

		uint32_t currentY = p_geometry.anchor.y;

		for (std::size_t row = 0; row < rowCountValue; ++row)
		{
			uint32_t currentX = p_geometry.anchor.x;

			spk::Geometry2D labelCellGeometry(
				{currentX, currentY},
				{labelColumnWidth, rowHeights[row]}
			);

			spk::Geometry2D fieldCellGeometry(
				{currentX + labelColumnWidth + _elementPadding.x, currentY},
				{fieldColumnWidth, rowHeights[row]}
			);

			Element *labelElement  = _elements[2 * row].get();
			if (labelElement != nullptr && labelElement->widget())
			{
				labelElement->widget()->setGeometry(labelCellGeometry);
			}

			Element *fieldElement  = _elements[2 * row + 1].get();
			if (fieldElement != nullptr && fieldElement->widget())
			{
				fieldElement->widget()->setGeometry(fieldCellGeometry);
			}

			currentY += static_cast<uint32_t>(rowHeights[row]) + _elementPadding.y;
		}
	}
}