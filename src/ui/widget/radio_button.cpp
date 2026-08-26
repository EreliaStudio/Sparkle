#include "ui/widget/radio_button.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace spk
{
	RadioButton::RadioButton(std::string name, Widget *parent) :
		Widget(std::move(name), parent),
		_indicator(this->name() + ".indicator", this),
		_label(this->name() + ".label", this)
	{
		applyStyle(defaultStyle);
		_label.setMaximalSize({std::numeric_limits<float>::max(), std::numeric_limits<float>::max()});
		_layout.addWidget(&_indicator, Layout::SizeSettings{Layout::SizePolicy::Fixed});
		auto *labelElement = _layout.addWidget(&_label, {Layout::SizePolicy::Extend, Layout::SizePolicy::Minimum});
		labelElement->setVerticalAlignment(Alignment::Vertical::Center);
		_indicatorContract = _indicator.subscribeToState([this](bool checked) {
			_stateProvider.trigger(checked);
			if (_group != nullptr)
			{
				_group->_onButtonStateChanged(*this, checked);
			}
		});
		setIndicatorSize(_indicatorSize);
		setSpacing(_spacing);
		activate();
	}

	RadioButton::RadioButton(std::string name, const SpriteSheet *iconset, Font *font, Widget *parent) :
		RadioButton(std::move(name), iconset, CheckableIconButton::DefaultUncheckedSpriteID, CheckableIconButton::DefaultCheckedSpriteID, font, parent)
	{
	}

	RadioButton::RadioButton(std::string name, const SpriteSheet *iconset, std::size_t uncheckedSpriteID, std::size_t checkedSpriteID, Font *font, Widget *parent) :
		RadioButton(std::move(name), parent)
	{
		_indicator.uncheckedButton().setIconset(iconset);
		_indicator.checkedButton().setIconset(iconset);
		_indicator.setUncheckedSpriteID(uncheckedSpriteID);
		_indicator.setCheckedSpriteID(checkedSpriteID);
		_label.setFont(font);
	}

	void RadioButton::applyStyle(const Style &style)
	{
		_indicator.applyStyle(style);
		_label.applyStyle(style);
		_updateSizeHint();
	}

	void RadioButton::_setCheckedFromGroup(bool checked)
	{
		_indicator.setChecked(checked);
	}
	void RadioButton::_updateSizeHint()
	{
		setSizeHint(_layout.sizeHint());
	}
	void RadioButton::_onGeometryChange()
	{
		_layout.setGeometry(Rect2D{Vector2Int{0, 0}, geometry().size});
	}
	void RadioButton::_onMouseButtonPressedEvent(MouseButtonPressedEvent &event)
	{
		if (event.record.button == Mouse::Button::Left && viewRegion().viewport.contains(event.device.position))
		{
			_pressed = true;
			event.consumed = true;
		}
	}
	void RadioButton::_onMouseButtonReleasedEvent(MouseButtonReleasedEvent &event)
	{
		if (event.record.button != Mouse::Button::Left || !_pressed)
		{
			return;
		}
		_pressed = false;
		if (viewRegion().viewport.contains(event.device.position))
		{
			setChecked(!isChecked());
			event.consumed = true;
		}
	}
	void RadioButton::setChecked(bool checked)
	{
		if (_group == nullptr)
		{
			_indicator.setChecked(checked);
		}
		else if (checked)
		{
			_group->select(*this);
		}
		else if (_group->selectedButton() == this)
		{
			_group->clearSelection();
		}
	}
	void RadioButton::setText(Font::Text text)
	{
		_label.setText(std::move(text));
	}
	void RadioButton::setText(std::string_view text)
	{
		_label.setText(text);
	}
	void RadioButton::setSpacing(unsigned int spacing)
	{
		_spacing = spacing;
		_layout.setElementPadding({spacing, 0});
		_updateSizeHint();
		_onGeometryChange();
	}
	void RadioButton::setIndicatorSize(const Vector2UInt &size)
	{
		_indicatorSize = size;
		const Vector2 fixed{static_cast<float>(size.x), static_cast<float>(size.y)};
		_indicator.setSizeHint({fixed, fixed, fixed});
		_updateSizeHint();
		_onGeometryChange();
	}
	bool RadioButton::isChecked() const noexcept
	{
		return _indicator.isChecked();
	}
	RadioButtonGroup *RadioButton::group() noexcept
	{
		return _group;
	}
	const RadioButtonGroup *RadioButton::group() const noexcept
	{
		return _group;
	}
	RadioButton::StateContract RadioButton::subscribeToState(StateCallback callback)
	{
		return _stateProvider.subscribe(std::move(callback));
	}
	CheckableIconButton &RadioButton::indicator() noexcept
	{
		return _indicator;
	}
	const CheckableIconButton &RadioButton::indicator() const noexcept
	{
		return _indicator;
	}
	TextLabel &RadioButton::label() noexcept
	{
		return _label;
	}
	const TextLabel &RadioButton::label() const noexcept
	{
		return _label;
	}

	RadioButtonGroup::RadioButtonGroup(std::string name, Widget *parent) :
		Widget(std::move(name), parent)
	{
		applyStyle(defaultStyle);
		activate();
	}
	RadioButtonGroup::RadioButtonGroup(std::string name, const SpriteSheet *iconset, Font *font, Widget *parent) :
		RadioButtonGroup(std::move(name), iconset, CheckableIconButton::DefaultUncheckedSpriteID, CheckableIconButton::DefaultCheckedSpriteID, font, parent)
	{
	}
	RadioButtonGroup::RadioButtonGroup(std::string name, const SpriteSheet *iconset, std::size_t uncheckedSpriteID, std::size_t checkedSpriteID, Font *font, Widget *parent) :
		RadioButtonGroup(std::move(name), parent)
	{
		setIconset(iconset);
		setSpriteIDs(uncheckedSpriteID, checkedSpriteID);
		setFont(font);
	}
	RadioButtonGroup::~RadioButtonGroup()
	{
		_layout.clear();
		for (auto &entry : _entries)
		{
			entry.button->_group = nullptr;
		}
		_entries.clear();
	}

	void RadioButtonGroup::applyStyle(const Style &style)
	{
		if (style.iconset != nullptr)
		{
			setIconset(style.iconset.get());
		}
		if (style.font != nullptr)
		{
			setFont(style.font.get());
		}
		for (auto &entry : _entries)
		{
			entry.button->applyStyle(style);
		}
	}

	RadioButtonGroup::Entry *RadioButtonGroup::_entry(RadioButton *button) noexcept
	{
		auto it = std::find_if(_entries.begin(), _entries.end(), [button](const Entry &entry) {
			return entry.button.get() == button;
		});
		return it == _entries.end() ? nullptr : &*it;
	}
	const RadioButtonGroup::Entry *RadioButtonGroup::_entry(const RadioButton *button) const noexcept
	{
		auto it = std::find_if(_entries.begin(), _entries.end(), [button](const Entry &entry) {
			return entry.button.get() == button;
		});
		return it == _entries.end() ? nullptr : &*it;
	}
	void RadioButtonGroup::_applySelection(RadioButton *button)
	{
		if (_selected == button)
		{
			return;
		}
		_selected = button;
		_synchronizing = true;
		for (auto &entry : _entries)
		{
			entry.button->_setCheckedFromGroup(entry.button.get() == _selected);
		}
		_synchronizing = false;
		_selectionProvider.trigger(selection());
	}
	void RadioButtonGroup::_onButtonStateChanged(RadioButton &button, bool checked)
	{
		if (_synchronizing)
		{
			return;
		}
		if (checked)
		{
			select(button);
		}
		else if (_selected == &button)
		{
			if (_allowNoSelection)
			{
				clearSelection();
			}
			else
			{
				_synchronizing = true;
				button._setCheckedFromGroup(true);
				_synchronizing = false;
			}
		}
	}
	void RadioButtonGroup::_updateSizeHint()
	{
		setSizeHint(_layout.sizeHint());
	}
	void RadioButtonGroup::_onGeometryChange()
	{
		_layout.setGeometry(Rect2D{Vector2Int{0, 0}, geometry().size});
	}

	RadioButton &RadioButtonGroup::insert(std::size_t column, std::size_t row, std::string_view text)
	{
		if (button(column, row) != nullptr)
		{
			throw std::invalid_argument("RadioButtonGroup cell is already occupied");
		}
		auto created = std::make_unique<RadioButton>(name() + ".button-" + std::to_string(_nextIdentifier++), this);
		created->_group = this;
		created->setText(text);
		created->setSpacing(_spacing);
		created->setIndicatorSize(_indicatorSize);
		if (_font != nullptr)
		{
			created->label().setFont(_font);
		}
		if (_iconset != nullptr)
		{
			created->indicator().uncheckedButton().setIconset(_iconset);
			created->indicator().checkedButton().setIconset(_iconset);
			created->indicator().setUncheckedSpriteID(_uncheckedSpriteID);
			created->indicator().setCheckedSpriteID(_checkedSpriteID);
		}
		RadioButton *result = created.get();
		_entries.push_back({{column, row}, std::move(created)});
		auto *element = _layout.setWidget(column, row, result, {Layout::SizePolicy::Extend, Layout::SizePolicy::Minimum});
		element->setVerticalAlignment(Alignment::Vertical::Center);
		_updateSizeHint();
		_onGeometryChange();
		if (_selected == nullptr && !_allowNoSelection)
		{
			_applySelection(result);
		}
		return *result;
	}
	void RadioButtonGroup::erase(std::size_t column, std::size_t row)
	{
		RadioButton *found = button(column, row);
		if (found != nullptr)
		{
			erase(*found);
		}
	}
	void RadioButtonGroup::erase(RadioButton &buttonToErase)
	{
		auto it = std::find_if(_entries.begin(), _entries.end(), [&buttonToErase](const Entry &entry) {
			return entry.button.get() == &buttonToErase;
		});
		if (it == _entries.end())
		{
			return;
		}
		const bool wasSelected = _selected == &buttonToErase;
		_layout.clearCell(it->cell.column, it->cell.row);
		it->button->_group = nullptr;
		_entries.erase(it);
		if (wasSelected)
		{
			_selected = nullptr;
			if (!_allowNoSelection && !_entries.empty())
			{
				_applySelection(_entries.front().button.get());
			}
			else
			{
				_selectionProvider.trigger(std::nullopt);
			}
		}
		_updateSizeHint();
		_onGeometryChange();
	}
	void RadioButtonGroup::clear()
	{
		const bool hadSelection = _selected != nullptr;
		_selected = nullptr;
		_layout.clear();
		for (auto &entry : _entries)
		{
			entry.button->_group = nullptr;
		}
		_entries.clear();
		_updateSizeHint();
		if (hadSelection)
		{
			_selectionProvider.trigger(std::nullopt);
		}
	}
	void RadioButtonGroup::select(RadioButton &buttonToSelect)
	{
		if (!contains(buttonToSelect))
		{
			throw std::invalid_argument("RadioButton does not belong to this group");
		}
		_applySelection(&buttonToSelect);
	}
	void RadioButtonGroup::select(std::size_t column, std::size_t row)
	{
		RadioButton *found = button(column, row);
		if (found == nullptr)
		{
			throw std::invalid_argument("RadioButtonGroup cell is empty");
		}
		select(*found);
	}
	void RadioButtonGroup::clearSelection()
	{
		if (_allowNoSelection)
		{
			_applySelection(nullptr);
		}
	}
	void RadioButtonGroup::setAllowNoSelection(bool allow)
	{
		_allowNoSelection = allow;
		if (!allow && _selected == nullptr && !_entries.empty())
		{
			_applySelection(_entries.front().button.get());
		}
	}
	void RadioButtonGroup::setSpacing(unsigned int spacing)
	{
		_spacing = spacing;
		for (auto &entry : _entries)
		{
			entry.button->setSpacing(spacing);
		}
	}
	void RadioButtonGroup::setIndicatorSize(const Vector2UInt &size)
	{
		_indicatorSize = size;
		for (auto &entry : _entries)
		{
			entry.button->setIndicatorSize(size);
		}
	}
	void RadioButtonGroup::setElementPadding(const Vector2UInt &padding)
	{
		_elementPadding = padding;
		_layout.setElementPadding(padding);
		_updateSizeHint();
		_onGeometryChange();
	}
	void RadioButtonGroup::setFont(Font *font)
	{
		if (font == nullptr)
		{
			throw std::invalid_argument("RadioButtonGroup font cannot be null");
		}
		_font = font;
		for (auto &entry : _entries)
		{
			entry.button->label().setFont(font);
		}
	}
	void RadioButtonGroup::setIconset(const SpriteSheet *iconset)
	{
		if (iconset == nullptr)
		{
			throw std::invalid_argument("RadioButtonGroup iconset cannot be null");
		}
		_iconset = iconset;
		for (auto &entry : _entries)
		{
			entry.button->indicator().uncheckedButton().setIconset(iconset);
			entry.button->indicator().checkedButton().setIconset(iconset);
		}
	}
	void RadioButtonGroup::setSpriteIDs(std::size_t unchecked, std::size_t checked)
	{
		_uncheckedSpriteID = unchecked;
		_checkedSpriteID = checked;
		for (auto &entry : _entries)
		{
			entry.button->indicator().setUncheckedSpriteID(unchecked);
			entry.button->indicator().setCheckedSpriteID(checked);
		}
	}
	bool RadioButtonGroup::contains(const RadioButton &buttonValue) const noexcept
	{
		return _entry(&buttonValue) != nullptr;
	}
	bool RadioButtonGroup::allowsNoSelection() const noexcept
	{
		return _allowNoSelection;
	}
	std::size_t RadioButtonGroup::size() const noexcept
	{
		return _entries.size();
	}
	std::size_t RadioButtonGroup::rowCount() const noexcept
	{
		return _layout.rowCount();
	}
	std::size_t RadioButtonGroup::columnCount() const noexcept
	{
		return _layout.columnCount();
	}
	unsigned int RadioButtonGroup::spacing() const noexcept
	{
		return _spacing;
	}
	const Vector2UInt &RadioButtonGroup::indicatorSize() const noexcept
	{
		return _indicatorSize;
	}
	const Vector2UInt &RadioButtonGroup::elementPadding() const noexcept
	{
		return _elementPadding;
	}
	RadioButton *RadioButtonGroup::selectedButton() noexcept
	{
		return _selected;
	}
	const RadioButton *RadioButtonGroup::selectedButton() const noexcept
	{
		return _selected;
	}
	std::optional<RadioButtonGroup::Cell> RadioButtonGroup::selectedCell() const noexcept
	{
		const Entry *entry = _entry(_selected);
		return entry == nullptr ? std::nullopt : std::optional<Cell>{entry->cell};
	}
	std::optional<RadioButtonGroup::Selection> RadioButtonGroup::selection() const noexcept
	{
		const Entry *entry = _entry(_selected);
		return entry == nullptr ? std::nullopt : std::optional<Selection>{{entry->cell, entry->button.get()}};
	}
	RadioButton *RadioButtonGroup::button(std::size_t column, std::size_t row) noexcept
	{
		for (auto &entry : _entries)
		{
			if (entry.cell == Cell{column, row})
			{
				return entry.button.get();
			}
		}
		return nullptr;
	}
	const RadioButton *RadioButtonGroup::button(std::size_t column, std::size_t row) const noexcept
	{
		for (const auto &entry : _entries)
		{
			if (entry.cell == Cell{column, row})
			{
				return entry.button.get();
			}
		}
		return nullptr;
	}
	RadioButton &RadioButtonGroup::buttonAt(std::size_t index)
	{
		return *_entries.at(index).button;
	}
	const RadioButton &RadioButtonGroup::buttonAt(std::size_t index) const
	{
		return *_entries.at(index).button;
	}
	RadioButtonGroup::SelectionContract RadioButtonGroup::subscribeToSelection(SelectionCallback callback)
	{
		return _selectionProvider.subscribe(std::move(callback));
	}
}
