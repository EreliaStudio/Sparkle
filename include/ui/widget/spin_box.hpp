#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include "design_pattern/contract_provider.hpp"
#include "ui/widget/icon_button.hpp"
#include "ui/widget/text_edit.hpp"

namespace spk
{
	template <typename TType>
	concept SpinBoxValue = std::is_arithmetic_v<TType>;

	template <SpinBoxValue TType>
	class SpinBox : public Widget
	{
	public:
		using EditionProvider = ContractProvider<TType>;
		using EditionCallback = typename EditionProvider::callback_type;
		using EditionContract = typename EditionProvider::Contract;

	private:
		IconButton _downButton;
		TextEdit _valueEdit;
		IconButton _upButton;
		IconButton::ClickContract _downClickContract;
		IconButton::ClickContract _upClickContract;
		std::optional<TType> _minimum;
		std::optional<TType> _maximum;
		TType _value{};
		TType _step{1};
		std::size_t _downSpriteID = 5;
		std::size_t _upSpriteID = 4;
		const SpriteSheet *_iconset = nullptr;
		EditionProvider _editionProvider;

		[[nodiscard]] TType _clamped(TType value) const
		{
			if (_minimum.has_value() && value < *_minimum)
			{
				value = *_minimum;
			}
			if (_maximum.has_value() && value > *_maximum)
			{
				value = *_maximum;
			}
			return value;
		}

		void _updateDisplay()
		{
			_valueEdit.setText(std::to_string(_value));
		}

		void _updateSizeHint() override
		{
			SizeHint hint = sizeHint();
			hint.minimal = {
				_downButton.minimalSize().x + _valueEdit.minimalSize().x + _upButton.minimalSize().x,
				std::max({_downButton.minimalSize().y, _valueEdit.minimalSize().y, _upButton.minimalSize().y})};
			hint.preferred = {
				_downButton.preferredSize().x + _valueEdit.preferredSize().x + _upButton.preferredSize().x,
				std::max({_downButton.preferredSize().y, _valueEdit.preferredSize().y, _upButton.preferredSize().y})};
			setSizeHint(hint);
		}

		void _onGeometryChange() override
		{
			const unsigned int buttonWidth = std::min(geometry().height, geometry().width / 2);
			_downButton.setGeometry({Vector2Int{0, 0}, Vector2UInt{buttonWidth, geometry().height}});
			_valueEdit.setGeometry({Vector2Int{static_cast<int>(buttonWidth), 0}, Vector2UInt{geometry().width - 2 * buttonWidth, geometry().height}});
			_upButton.setGeometry({Vector2Int{static_cast<int>(geometry().width - buttonWidth), 0}, Vector2UInt{buttonWidth, geometry().height}});
		}

	public:
		explicit SpinBox(std::string name, Widget *parent = nullptr) :
			Widget(std::move(name), parent),
			_downButton(this->name() + ".down", this),
			_valueEdit(this->name() + ".value", this),
			_upButton(this->name() + ".up", this)
		{
			_downButton.setText("-");
			_upButton.setText("+");
			_valueEdit.setPlaceholder("...");
			_valueEdit.disableEdit();
			_downClickContract = _downButton.subscribeToClick([this]() {
				decrease();
			});
			_upClickContract = _upButton.subscribeToClick([this]() {
				increase();
			});
			_updateDisplay();
			_updateSizeHint();
			activate();
		}

		SpinBox(std::string name, Font *font, Widget *parent = nullptr) :
			SpinBox(std::move(name), parent)
		{
			_valueEdit.setFont(font);
		}

		SpinBox(std::string name, const SpriteSheet *iconset, Font *font, Widget *parent = nullptr) :
			SpinBox(std::move(name), font, parent)
		{
			setIconset(iconset);
		}

		[[nodiscard]] EditionContract subscribeToEdition(EditionCallback callback)
		{
			return _editionProvider.subscribe(std::move(callback));
		}

		void setValue(TType value)
		{
			value = _clamped(value);
			if (_value == value)
			{
				return;
			}
			_value = value;
			_updateDisplay();
			_editionProvider.trigger(_value);
		}

		void setStep(TType step)
		{
			_step = step;
		}

		void setMinimum(TType minimum)
		{
			_minimum = minimum;
			setValue(_value);
		}

		void setMaximum(TType maximum)
		{
			_maximum = maximum;
			setValue(_value);
		}

		void removeMinimum()
		{
			_minimum.reset();
		}

		void removeMaximum()
		{
			_maximum.reset();
		}

		void removeLimits()
		{
			_minimum.reset();
			_maximum.reset();
		}

		void increase()
		{
			setValue(static_cast<TType>(_value + _step));
		}

		void decrease()
		{
			setValue(static_cast<TType>(_value - _step));
		}

		void setIconset(const SpriteSheet *iconset)
		{
			_iconset = iconset;
			_downButton.setIconSpriteID(_downSpriteID);
			_upButton.setIconSpriteID(_upSpriteID);
			_downButton.setIconset(iconset);
			_upButton.setIconset(iconset);
			_downButton.setText("");
			_upButton.setText("");
		}

		void setButtonSpriteIDs(std::size_t downSpriteID, std::size_t upSpriteID)
		{
			_downSpriteID = downSpriteID;
			_upSpriteID = upSpriteID;
			_downButton.setIconSpriteID(downSpriteID);
			_upButton.setIconSpriteID(upSpriteID);
		}

		[[nodiscard]] TType value() const noexcept
		{
			return _value;
		}

		[[nodiscard]] TType step() const noexcept
		{
			return _step;
		}

		[[nodiscard]] const std::optional<TType> &minimum() const noexcept
		{
			return _minimum;
		}

		[[nodiscard]] const std::optional<TType> &maximum() const noexcept
		{
			return _maximum;
		}

		[[nodiscard]] const SpriteSheet *iconset() const noexcept
		{
			return _iconset;
		}

		[[nodiscard]] IconButton &downButton() noexcept
		{
			return _downButton;
		}

		[[nodiscard]] const IconButton &downButton() const noexcept
		{
			return _downButton;
		}

		[[nodiscard]] TextEdit &valueEdit() noexcept
		{
			return _valueEdit;
		}

		[[nodiscard]] const TextEdit &valueEdit() const noexcept
		{
			return _valueEdit;
		}

		[[nodiscard]] IconButton &upButton() noexcept
		{
			return _upButton;
		}

		[[nodiscard]] const IconButton &upButton() const noexcept
		{
			return _upButton;
		}
	};
}
