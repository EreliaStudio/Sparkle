#include "ui/widget/check_box.hpp"

#include <limits>
#include <utility>

namespace spk
{
	CheckBox::CheckBox(std::string name, Widget *parent) :
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
		});
		setIndicatorSize(_indicatorSize);
		setSpacing(_spacing);
		activate();
	}

	CheckBox::CheckBox(std::string name, const SpriteSheet *iconset, Font *font, Widget *parent) :
		CheckBox(std::move(name), iconset, CheckableIconButton::DefaultUncheckedSpriteID, CheckableIconButton::DefaultCheckedSpriteID, font, parent)
	{
	}

	CheckBox::CheckBox(std::string name, const SpriteSheet *iconset, std::size_t uncheckedSpriteID, std::size_t checkedSpriteID, Font *font, Widget *parent) :
		CheckBox(std::move(name), parent)
	{
		_indicator.uncheckedButton().setIconset(iconset);
		_indicator.checkedButton().setIconset(iconset);
		_indicator.setUncheckedSpriteID(uncheckedSpriteID);
		_indicator.setCheckedSpriteID(checkedSpriteID);
		_label.setFont(font);
	}

	void CheckBox::applyStyle(const Style &style)
	{
		_indicator.applyStyle(style);
		_label.applyStyle(style);
		_updateSizeHint();
	}

	void CheckBox::_updateSizeHint()
	{
		setSizeHint(_layout.sizeHint());
	}
	void CheckBox::_onGeometryChange()
	{
		_layout.setGeometry(Rect2D{Vector2Int{0, 0}, geometry().size});
	}

	void CheckBox::_onMouseButtonPressedEvent(MouseButtonPressedEvent &event)
	{
		if (event.record.button == Mouse::Button::Left && viewRegion().viewport.contains(event.device.position))
		{
			_pressed = true;
			event.consumed = true;
		}
	}

	void CheckBox::_onMouseButtonReleasedEvent(MouseButtonReleasedEvent &event)
	{
		if (event.record.button != Mouse::Button::Left || !_pressed)
		{
			return;
		}
		_pressed = false;
		if (viewRegion().viewport.contains(event.device.position))
		{
			toggle();
			event.consumed = true;
		}
	}

	void CheckBox::setChecked(bool checked)
	{
		_indicator.setChecked(checked);
	}
	void CheckBox::toggle()
	{
		_indicator.toggle();
	}
	void CheckBox::setText(Font::Text text)
	{
		_label.setText(std::move(text));
	}
	void CheckBox::setText(std::string_view text)
	{
		_label.setText(text);
	}
	void CheckBox::setSpacing(unsigned int spacing)
	{
		_spacing = spacing;
		_layout.setElementPadding({_spacing, 0});
		_updateSizeHint();
		_onGeometryChange();
	}
	void CheckBox::setIndicatorSize(const Vector2UInt &size)
	{
		_indicatorSize = size;
		const Vector2 value{static_cast<float>(size.x), static_cast<float>(size.y)};
		_indicator.setSizeHint({value, value, value});
		_updateSizeHint();
		_onGeometryChange();
	}
	bool CheckBox::isChecked() const noexcept
	{
		return _indicator.isChecked();
	}
	unsigned int CheckBox::spacing() const noexcept
	{
		return _spacing;
	}
	const Vector2UInt &CheckBox::indicatorSize() const noexcept
	{
		return _indicatorSize;
	}
	CheckBox::StateContract CheckBox::subscribeToState(StateCallback callback)
	{
		return _stateProvider.subscribe(std::move(callback));
	}
	CheckableIconButton &CheckBox::indicator() noexcept
	{
		return _indicator;
	}
	const CheckableIconButton &CheckBox::indicator() const noexcept
	{
		return _indicator;
	}
	TextLabel &CheckBox::label() noexcept
	{
		return _label;
	}
	const TextLabel &CheckBox::label() const noexcept
	{
		return _label;
	}
}
