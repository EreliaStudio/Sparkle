#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "design_pattern/contract_provider.hpp"
#include "ui/layout/grid_layout.hpp"
#include "ui/layout/linear_layout.hpp"
#include "ui/widget/checkable_icon_button.hpp"
#include "ui/widget/text_label.hpp"

namespace spk
{
	class RadioButtonGroup;

	class RadioButton : public Widget
	{
		friend class RadioButtonGroup;

	public:
		using StateProvider = ContractProvider<bool>;
		using StateCallback = StateProvider::callback_type;
		using StateContract = StateProvider::Contract;

	private:
		CheckableIconButton _indicator;
		TextLabel _label;
		HorizontalLayout _layout;
		CheckableIconButton::StateContract _indicatorContract;
		StateProvider _stateProvider;
		RadioButtonGroup *_group = nullptr;
		unsigned int _spacing = 4;
		Vector2UInt _indicatorSize{16, 16};
		bool _pressed = false;

		void _setCheckedFromGroup(bool checked);
		void _updateSizeHint() override;
		void _onGeometryChange() override;
		void _onMouseButtonPressedEvent(MouseButtonPressedEvent &event) override;
		void _onMouseButtonReleasedEvent(MouseButtonReleasedEvent &event) override;

	public:
		explicit RadioButton(std::string name, Widget *parent = nullptr);
		RadioButton(std::string name, const SpriteSheet *iconset, Font *font, Widget *parent = nullptr);
		RadioButton(std::string name, const SpriteSheet *iconset, std::size_t uncheckedSpriteID, std::size_t checkedSpriteID, Font *font, Widget *parent = nullptr);
		void applyStyle(const Style &style) override;

		void setChecked(bool checked);
		void setText(Font::Text text);
		void setText(std::string_view text);
		void setSpacing(unsigned int spacing);
		void setIndicatorSize(const Vector2UInt &size);
		[[nodiscard]] bool isChecked() const noexcept;
		[[nodiscard]] RadioButtonGroup *group() noexcept;
		[[nodiscard]] const RadioButtonGroup *group() const noexcept;
		[[nodiscard]] StateContract subscribeToState(StateCallback callback);
		[[nodiscard]] CheckableIconButton &indicator() noexcept;
		[[nodiscard]] const CheckableIconButton &indicator() const noexcept;
		[[nodiscard]] TextLabel &label() noexcept;
		[[nodiscard]] const TextLabel &label() const noexcept;
	};

	class RadioButtonGroup : public Widget
	{
		friend class RadioButton;

	public:
		struct Cell
		{
			std::size_t column = 0;
			std::size_t row = 0;
			bool operator==(const Cell &) const = default;
		};
		struct Selection
		{
			Cell cell;
			RadioButton *button = nullptr;
			bool operator==(const Selection &) const = default;
		};
		using SelectionProvider = ContractProvider<std::optional<Selection>>;
		using SelectionCallback = SelectionProvider::callback_type;
		using SelectionContract = SelectionProvider::Contract;

	private:
		struct Entry
		{
			Cell cell;
			std::unique_ptr<RadioButton> button;
		};

		GridLayout _layout;
		std::vector<Entry> _entries;
		RadioButton *_selected = nullptr;
		bool _allowNoSelection = false;
		bool _synchronizing = false;
		unsigned int _spacing = 4;
		Vector2UInt _indicatorSize{16, 16};
		Vector2UInt _elementPadding{0, 0};
		Font *_font = nullptr;
		const SpriteSheet *_iconset = nullptr;
		std::size_t _uncheckedSpriteID = CheckableIconButton::DefaultUncheckedSpriteID;
		std::size_t _checkedSpriteID = CheckableIconButton::DefaultCheckedSpriteID;
		std::size_t _nextIdentifier = 0;
		SelectionProvider _selectionProvider;

		void _onButtonStateChanged(RadioButton &button, bool checked);
		void _applySelection(RadioButton *button);
		[[nodiscard]] Entry *_entry(RadioButton *button) noexcept;
		[[nodiscard]] const Entry *_entry(const RadioButton *button) const noexcept;
		void _updateSizeHint() override;
		void _onGeometryChange() override;

	public:
		explicit RadioButtonGroup(std::string name, Widget *parent = nullptr);
		RadioButtonGroup(std::string name, const SpriteSheet *iconset, Font *font, Widget *parent = nullptr);
		RadioButtonGroup(std::string name, const SpriteSheet *iconset, std::size_t uncheckedSpriteID, std::size_t checkedSpriteID, Font *font, Widget *parent = nullptr);
		~RadioButtonGroup() override;
		void applyStyle(const Style &style) override;

		RadioButton &insert(std::size_t column, std::size_t row, std::string_view text);
		void erase(std::size_t column, std::size_t row);
		void erase(RadioButton &button);
		void clear();
		void select(RadioButton &button);
		void select(std::size_t column, std::size_t row);
		void clearSelection();
		void setAllowNoSelection(bool allow);
		void setSpacing(unsigned int spacing);
		void setIndicatorSize(const Vector2UInt &size);
		void setElementPadding(const Vector2UInt &padding);
		void setFont(Font *font);
		void setIconset(const SpriteSheet *iconset);
		void setSpriteIDs(std::size_t uncheckedSpriteID, std::size_t checkedSpriteID);

		[[nodiscard]] bool contains(const RadioButton &button) const noexcept;
		[[nodiscard]] bool allowsNoSelection() const noexcept;
		[[nodiscard]] std::size_t size() const noexcept;
		[[nodiscard]] std::size_t rowCount() const noexcept;
		[[nodiscard]] std::size_t columnCount() const noexcept;
		[[nodiscard]] unsigned int spacing() const noexcept;
		[[nodiscard]] const Vector2UInt &indicatorSize() const noexcept;
		[[nodiscard]] const Vector2UInt &elementPadding() const noexcept;
		[[nodiscard]] RadioButton *selectedButton() noexcept;
		[[nodiscard]] const RadioButton *selectedButton() const noexcept;
		[[nodiscard]] std::optional<Cell> selectedCell() const noexcept;
		[[nodiscard]] std::optional<Selection> selection() const noexcept;
		[[nodiscard]] RadioButton *button(std::size_t column, std::size_t row) noexcept;
		[[nodiscard]] const RadioButton *button(std::size_t column, std::size_t row) const noexcept;
		[[nodiscard]] RadioButton &buttonAt(std::size_t insertionIndex);
		[[nodiscard]] const RadioButton &buttonAt(std::size_t insertionIndex) const;
		[[nodiscard]] SelectionContract subscribeToSelection(SelectionCallback callback);
	};
}
