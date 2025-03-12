#pragma once

#include <optional>

#include "widget/spk_push_button.hpp"
#include "widget/spk_text_edit.hpp"

#include "structure/design_pattern/spk_observable_value.hpp"

namespace spk
{
	template <typename TType>
	class SpinBox : public spk::Widget
	{
	private:
		spk::PushButton _upButton;
		spk::PushButton::Contract _upButtonContract;
		spk::TextEdit _valueEdit;
		spk::PushButton _downButton;
		spk::PushButton::Contract _downButtonContract;

		std::optional<TType> _minLimit;
		std::optional<TType> _maxLimit;
		spk::ObservableValue<TType> _value;
		spk::ObservableValue<TType>::Contract _onValueEditionContract;
		TType _step = static_cast<TType>(1);

		void _onGeometryChange() override
		{
			spk::Vector2UInt buttonSize = { std::max(16u, geometry().size.y), geometry().size.y };

			spk::Vector2UInt editSize = {
				geometry().size.x - (buttonSize.x * 2) - 6,
				geometry().size.y
			};

			_valueEdit.setFontSize({static_cast<size_t>((geometry().height - _valueEdit.cornerSize().y * 2)), 0});

			_valueEdit.setGeometry({ 0, 0 }, editSize);
			_downButton.setGeometry({editSize.x + 3, 0}, buttonSize);
			_upButton.setGeometry({editSize.x + 3 + buttonSize.x + 3, 0}, buttonSize);
		}

	public:
		SpinBox(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
			spk::Widget(p_name, p_parent),
			_upButton(p_name + L" - Raise button", this),
			_valueEdit(p_name + L" - Value edit", this),
			_downButton(p_name + L" - Lower button", this),
			_value(static_cast<TType>(0)),
			_onValueEditionContract(_value.subscribe([&]() { _valueEdit.setText(std::to_wstring(_value.get())); }))
		{
			_upButton.setTextAlignment(spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Centered);
			_upButtonContract = _upButton.subscribe([&]() {
				if (_maxLimit.has_value() == false)
				{
					_value += _step;
				}
				else if (_value != _maxLimit.value())
				{
					_value = std::min(_value + _step, _maxLimit.value());
				}
				});
			_upButton.activate();

			_valueEdit.setPlaceholder(L"...");
			_valueEdit.setTextAlignment(spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Centered);
			_valueEdit.activate();

			_downButton.setTextAlignment(spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Centered);
			_downButtonContract = _downButton.subscribe([&](){
				if (_minLimit.has_value() == false)
				{
					_value -= _step;
				}
				else if (_value != _minLimit.value())
				{
					_value = std::max(_value - _step, _minLimit.value());
				}
				});
			_downButton.activate();

			_value.trigger();

			setIconSet(spk::Widget::defaultIconset());
		}

		void setIconSet(spk::SafePointer<const spk::SpriteSheet> p_iconSet)
		{
			_upButton.setText(L"");
			_upButton.setIconset(p_iconSet);
			_upButton.setIcon(p_iconSet->sprite(4));

			_downButton.setIconset(p_iconSet);
			_downButton.setText(L"");
			_downButton.setIcon(p_iconSet->sprite(5));
		}

		void setNineSlice(spk::SafePointer<const spk::SpriteSheet> p_spriteSheet)
		{
			_upButton.setNineSlice(p_spriteSheet);
			_valueEdit.setNineSlice(p_spriteSheet);
			_downButton.setNineSlice(p_spriteSheet);
		}

		void setCornerSize(const spk::Vector2UInt& p_cornerSize)
		{
			_upButton.setCornerSize(p_cornerSize);
			_valueEdit.setCornerSize(p_cornerSize);
			_downButton.setCornerSize(p_cornerSize);
		}

		void setValue(TType p_value)
		{
			_value.set(p_value);
		}

		TType getValue() const
		{
			return _value.get();
		}

		void setStep(TType p_step)
		{
			_step = p_step;
		}

		void setMinimalLimit(TType p_minimalValue)
		{
			_minLimit = p_minimalValue;
		}

		void setMaximalLimit(TType p_maximalValue)
		{
			_maxLimit = p_maximalValue;
		}

		void setLimits(const TType& p_minimalValue, const TType& p_maximalValue)
		{
			setMinimalLimit(p_minimalValue);
			setMaximalLimit(p_maximalValue);
		}

		void removeLimits()
		{
			_minLimit.reset();
			_maxLimit.reset();
		}

		bool isEditEnable() const
		{
			return (_valueEdit.isEditEnable());
		}

		void enableEdit()
		{
			_valueEdit.disableEdit();
		}

		void disableEdit()
		{
			_valueEdit.disableEdit();
		}
	};
}