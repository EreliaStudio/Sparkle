#pragma once

#include "ui/layout/grid_layout.hpp"

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
			SizeSettings labelSettings = SizePolicy::Fixed,
			SizeSettings fieldSettings = {SizePolicy::Extend, SizePolicy::Fixed});

		void removeRow(const FormElement &row);
		[[nodiscard]] std::size_t nbRow() const noexcept;
	};
}
