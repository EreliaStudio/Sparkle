#pragma once

#include "widget/spk_layout.hpp"

#include "widget/spk_text_label.hpp"

namespace spk
{
	class FormLayout : public Layout
	{
	public:
		template<typename WidgetT>
		struct Row
		{
			TextLabel label;
			WidgetT   field;

			Row(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
				label(p_name + L"Label", p_parent),
				field(p_name + L"Field", p_parent)
			{
			}

			void activate()
			{
				label.activate();
				field.activate();
			}

			void deactivate()
			{
				label.deactivate();
				field.deactivate();
			}

			spk::Vector2UInt minimalSize() const
			{
				spk::Vector2UInt labelSize = label.minimalSize();
				spk::Vector2UInt fieldSize = field.minimalSize();

				return {labelSize.x + fieldSize.x, std::max(labelSize.y, fieldSize.y)};
			}
		};

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

		template <typename WidgetType>
		FormElement addRow(Row<WidgetType>& p_row,
						const SizePolicy &p_labelPolicy = SizePolicy::Minimum,
						const SizePolicy &p_fieldPolicy = SizePolicy::Extend)
		{
			return FormElement{
				addWidget(&(p_row.label), p_labelPolicy),
				addWidget(&(p_row.field), p_fieldPolicy)};
		}

		void removeRow(const FormElement& p_row);

		size_t nbRow() const;

		void setGeometry(const spk::Geometry2D &p_geometry) override;

		spk::Vector2UInt minimalSize() const;
	};
}