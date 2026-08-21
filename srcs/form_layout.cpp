#include "form_layout.hpp"

#include <stdexcept>

namespace spk
{
	FormLayout::FormElement FormLayout::addRow(Widget *labelWidget, Widget *fieldWidget, SizePolicy labelPolicy, SizePolicy fieldPolicy)
	{
		if (labelWidget == nullptr || fieldWidget == nullptr)
		{
			throw std::invalid_argument("FormLayout cannot hold a null widget");
		}
		const std::size_t row = rowCount();
		Element *label = GridLayoutFixedColumns<2>::setWidget(0, row, labelWidget, labelPolicy);
		Element *field = GridLayoutFixedColumns<2>::setWidget(1, row, fieldWidget, fieldPolicy);
		label->setHorizontalAlignment(HorizontalAlignment::Right);
		label->setVerticalAlignment(VerticalAlignment::Center);
		field->setVerticalAlignment(VerticalAlignment::Center);
		return {label, field};
	}

	void FormLayout::removeRow(const FormElement &row)
	{
		for (std::size_t index = 0; index < rowCount(); ++index)
		{
			if (element(0, index) != row.label || element(1, index) != row.field)
			{
				continue;
			}
			GridLayout::removeRow(index);
			return;
		}
	}

	std::size_t FormLayout::nbRow() const noexcept
	{
		return rowCount();
	}
}
