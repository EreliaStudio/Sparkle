#pragma once

#include "grid_layout.hpp"

namespace spk
{
	class FormLayout : public GridLayoutFixedColumns<2>
	{
	private:
		using GridLayoutFixedColumns<2>::addEmptyRow;
		using GridLayoutFixedColumns<2>::clearCell;
		using GridLayoutFixedColumns<2>::removeRow;
		using GridLayoutFixedColumns<2>::setLayout;
		using GridLayoutFixedColumns<2>::setWidget;

	public:
		struct FormElement
		{
			Element *label = nullptr;
			Element *field = nullptr;
		};

		FormElement addRow(
			Widget *labelWidget,
			Widget *fieldWidget,
			SizePolicy labelPolicy = SizePolicy::Fixed,
			SizePolicy fieldPolicy = SizePolicy::HorizontalExtend);

		void removeRow(const FormElement &row);
		[[nodiscard]] std::size_t nbRow() const noexcept;
	};
}
