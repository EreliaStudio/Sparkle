#pragma once

#include "widget/spk_layout.hpp"

namespace spk
{
	class FormLayout : public Layout
	{
	private:
		using Layout::addWidget;
		using Layout::removeWidget;

	public:
		struct FormElement
		{
			spk::SafePointer<Element> labelElement;
			spk::SafePointer<Element> fieldElement;
		};

		FormLayout() = default;

		FormElement addRow(spk::SafePointer<spk::Widget> p_labelWidget,
						spk::SafePointer<spk::Widget> p_fieldWidget,
						const SizePolicy &p_labelPolicy = SizePolicy::Minimum,
						const SizePolicy &p_fieldPolicy = SizePolicy::Extend);

		void removeRow(const FormElement& p_row);

		size_t nbRow() const;

		void setGeometry(const spk::Geometry2D &p_geometry) override;
	};
}