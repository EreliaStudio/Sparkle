#pragma once

#include <algorithm>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <optional>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

#include "design_pattern/contract_provider.hpp"
#include "ui/widget/icon_button.hpp"
#include "ui/widget/text_edit.hpp"

namespace spk
{
	template <typename TType>
	concept NumericSpinBoxValue = std::is_arithmetic_v<TType> && !std::same_as<std::remove_cv_t<TType>, bool>;

	template <NumericSpinBoxValue TType>
	class NumericSpinBox : public Widget
	{
	public:
		using EditionProvider = ContractProvider<TType>;
		using EditionCallback = typename EditionProvider::callback_type;
		using EditionContract = typename EditionProvider::Contract;

	private:
		TextEdit _valueEdit;
		IconButton _lowerButton;
		IconButton _raiseButton;
		TextEdit::EditionContract _textEditionContract;
		IconButton::ClickContract _lowerClickContract;
		IconButton::ClickContract _raiseClickContract;
		TType _value{};
		TType _step{1};
		bool _synchronizing = false;
		std::size_t _lowerSpriteID = 5;
		std::size_t _raiseSpriteID = 4;
		const SpriteSheet *_iconset = nullptr;
		EditionProvider _editionProvider;

		[[nodiscard]] static bool _ascii(const Font::Text &text) noexcept
		{
			return std::ranges::all_of(text, [](char32_t glyph) {
				return glyph <= 127;
			});
		}

		[[nodiscard]] static std::string _narrow(const Font::Text &text)
		{
			std::string result;
			result.reserve(text.size());
			for (char32_t glyph : text)
			{
				result.push_back(static_cast<char>(glyph));
			}
			return result;
		}

		[[nodiscard]] static bool _undefined(const Font::Text &text) noexcept
		{
			if (text.empty())
			{
				return true;
			}
			if constexpr (std::is_signed_v<TType>)
			{
				if (text == U"-" || text == U"+")
				{
					return true;
				}
			}
			else if (text == U"+")
			{
				return true;
			}
			if constexpr (std::is_floating_point_v<TType>)
			{
				return text == U"." || text == U"-." || text == U"+.";
			}
			return false;
		}

		[[nodiscard]] static std::optional<TType> _parse(const Font::Text &text)
		{
			if (!_ascii(text) || _undefined(text))
			{
				return std::nullopt;
			}
			std::string source = _narrow(text);
			if constexpr (std::is_unsigned_v<TType>)
			{
				if (!source.empty() && source.front() == '-')
				{
					return std::nullopt;
				}
			}

			const char *begin = source.data();
			const char *end = begin + source.size();
			if (begin != end && *begin == '+')
			{
				++begin;
			}
			TType result{};
			const auto [parsedEnd, error] = std::from_chars(begin, end, result);
			if (error != std::errc{} || parsedEnd != end)
			{
				return std::nullopt;
			}
			return result;
		}

		[[nodiscard]] static TextEdit::ValidationState _validation(const Font::Text &text)
		{
			if (!_ascii(text))
			{
				return TextEdit::ValidationState::Invalid;
			}
			if (_undefined(text))
			{
				return TextEdit::ValidationState::Undefined;
			}
			return _parse(text).has_value() ? TextEdit::ValidationState::Valid : TextEdit::ValidationState::Invalid;
		}

		void _synchronizeText()
		{
			_synchronizing = true;
			_valueEdit.setText(std::to_string(_value));
			_synchronizing = false;
		}

		void _onTextEdition(const Font::Text &text)
		{
			if (_synchronizing || _validation(text) != TextEdit::ValidationState::Valid)
			{
				return;
			}
			const std::optional<TType> parsed = _parse(text);
			if (!parsed.has_value() || *parsed == _value)
			{
				return;
			}
			_value = *parsed;
			_editionProvider.trigger(_value);
		}

		void _updateSizeHint() override
		{
			SizeHint hint = sizeHint();
			hint.minimal = {
				_valueEdit.minimalSize().x + _lowerButton.minimalSize().x + _raiseButton.minimalSize().x,
				std::max({_valueEdit.minimalSize().y, _lowerButton.minimalSize().y, _raiseButton.minimalSize().y})};
			hint.preferred = {
				_valueEdit.preferredSize().x + _lowerButton.preferredSize().x + _raiseButton.preferredSize().x,
				std::max({_valueEdit.preferredSize().y, _lowerButton.preferredSize().y, _raiseButton.preferredSize().y})};
			setSizeHint(hint);
		}

		void _onGeometryChange() override
		{
			const unsigned int buttonWidth = std::min(geometry().height, geometry().width / 2);
			const unsigned int editWidth = geometry().width - 2 * buttonWidth;
			_valueEdit.setGeometry({Vector2Int{0, 0}, Vector2UInt{editWidth, geometry().height}});
			_lowerButton.setGeometry({Vector2Int{static_cast<int>(editWidth), 0}, Vector2UInt{buttonWidth, geometry().height}});
			_raiseButton.setGeometry({Vector2Int{static_cast<int>(editWidth + buttonWidth), 0}, Vector2UInt{buttonWidth, geometry().height}});
		}

	public:
		explicit NumericSpinBox(std::string name, Widget *parent = nullptr) :
			Widget(std::move(name), parent),
			_valueEdit(this->name() + ".value", this),
			_lowerButton(this->name() + ".lower", this),
			_raiseButton(this->name() + ".raise", this)
		{
			applyStyle(defaultStyle);
			_lowerButton.setText("-");
			_raiseButton.setText("+");
			_valueEdit.setPlaceholder("...");
			_valueEdit.setValidationCallback([](const Font::Text &text) {
				return _validation(text);
			});
			_textEditionContract = _valueEdit.subscribeToEdition([this](const Font::Text &text) {
				_onTextEdition(text);
			});
			_lowerClickContract = _lowerButton.subscribeToClick([this]() {
				decrease();
			});
			_raiseClickContract = _raiseButton.subscribeToClick([this]() {
				increase();
			});
			_synchronizeText();
			_updateSizeHint();
			activate();
		}

		void applyStyle(const Style &style) override
		{
			_valueEdit.applyStyle(style);
			_lowerButton.applyStyle(style);
			_raiseButton.applyStyle(style);
			_updateSizeHint();
		}

		NumericSpinBox(std::string name, Font *font, Widget *parent = nullptr) :
			NumericSpinBox(std::move(name), parent)
		{
			_valueEdit.setFont(font);
		}

		NumericSpinBox(std::string name, const SpriteSheet *iconset, Font *font, Widget *parent = nullptr) :
			NumericSpinBox(std::move(name), font, parent)
		{
			setIconset(iconset);
		}

		[[nodiscard]] EditionContract subscribeToEdition(EditionCallback callback)
		{
			return _editionProvider.subscribe(std::move(callback));
		}

		void setValue(TType value)
		{
			if (_value == value)
			{
				return;
			}
			_value = value;
			_synchronizeText();
			_editionProvider.trigger(_value);
		}

		void setStep(TType step)
		{
			_step = step;
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
			_lowerButton.setIconSpriteID(_lowerSpriteID);
			_raiseButton.setIconSpriteID(_raiseSpriteID);
			_lowerButton.setIconset(iconset);
			_raiseButton.setIconset(iconset);
			_lowerButton.setText("");
			_raiseButton.setText("");
		}

		void setButtonSpriteIDs(std::size_t lowerSpriteID, std::size_t raiseSpriteID)
		{
			_lowerSpriteID = lowerSpriteID;
			_raiseSpriteID = raiseSpriteID;
			_lowerButton.setIconSpriteID(lowerSpriteID);
			_raiseButton.setIconSpriteID(raiseSpriteID);
		}

		[[nodiscard]] TType value() const noexcept
		{
			return _value;
		}

		[[nodiscard]] TType step() const noexcept
		{
			return _step;
		}

		[[nodiscard]] const SpriteSheet *iconset() const noexcept
		{
			return _iconset;
		}

		[[nodiscard]] TextEdit &valueEdit() noexcept
		{
			return _valueEdit;
		}

		[[nodiscard]] const TextEdit &valueEdit() const noexcept
		{
			return _valueEdit;
		}

		[[nodiscard]] IconButton &lowerButton() noexcept
		{
			return _lowerButton;
		}

		[[nodiscard]] const IconButton &lowerButton() const noexcept
		{
			return _lowerButton;
		}

		[[nodiscard]] IconButton &raiseButton() noexcept
		{
			return _raiseButton;
		}

		[[nodiscard]] const IconButton &raiseButton() const noexcept
		{
			return _raiseButton;
		}
	};

	using FloatSpinBox = NumericSpinBox<float>;
	using IntSpinBox = NumericSpinBox<int>;
	using UnsignedIntSpinBox = NumericSpinBox<unsigned int>;
}
