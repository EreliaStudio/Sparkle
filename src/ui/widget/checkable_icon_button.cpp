#include "ui/widget/checkable_icon_button.hpp"

#include <algorithm>
#include <utility>

namespace spk
{
	CheckableIconButton::CheckableIconButton(std::string name, Widget *parent) :
		Widget(std::move(name), parent),
		_uncheckedButton(this->name() + ".unchecked", this),
		_checkedButton(this->name() + ".checked", this)
	{
		_uncheckedButton.setIconSpriteID(DefaultUncheckedSpriteID);
		_checkedButton.setIconSpriteID(DefaultCheckedSpriteID);
		applyStyle(defaultStyle);
		_uncheckedClickContract = _uncheckedButton.subscribeToClick([this]() {
			toggle();
		});
		_checkedClickContract = _checkedButton.subscribeToClick([this]() {
			toggle();
		});
		_applyState();
		_updateSizeHint();
		activate();
	}

	CheckableIconButton::CheckableIconButton(std::string name, const SpriteSheet *iconset, Widget *parent) :
		CheckableIconButton(std::move(name), iconset, DefaultUncheckedSpriteID, DefaultCheckedSpriteID, parent)
	{
	}

	CheckableIconButton::CheckableIconButton(std::string name, const SpriteSheet *iconset, std::size_t uncheckedSpriteID, std::size_t checkedSpriteID, Widget *parent) :
		CheckableIconButton(std::move(name), parent)
	{
		_uncheckedButton.setIconSpriteID(uncheckedSpriteID);
		_checkedButton.setIconSpriteID(checkedSpriteID);
		_uncheckedButton.setIconset(iconset);
		_checkedButton.setIconset(iconset);
		_updateSizeHint();
	}

	void CheckableIconButton::applyStyle(const Style &style)
	{
		_uncheckedButton.applyStyle(style);
		_checkedButton.applyStyle(style);
		_updateSizeHint();
	}

	void CheckableIconButton::_applyState()
	{
		if (_checked)
		{
			_uncheckedButton.deactivate();
			_checkedButton.activate();
		}
		else
		{
			_checkedButton.deactivate();
			_uncheckedButton.activate();
		}
	}

	void CheckableIconButton::_updateSizeHint()
	{
		const auto maximum = [](const Vector2 &lhs, const Vector2 &rhs) {
			return Vector2{std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y)};
		};
		setSizeHint(SizeHint{.minimal = maximum(_uncheckedButton.minimalSize(), _checkedButton.minimalSize()), .maximal = maximum(_uncheckedButton.maximalSize(), _checkedButton.maximalSize()), .preferred = maximum(_uncheckedButton.preferredSize(), _checkedButton.preferredSize())});
	}

	void CheckableIconButton::_onGeometryChange()
	{
		const Rect2D fill{Vector2Int{0, 0}, geometry().size};
		_uncheckedButton.setGeometry(fill);
		_checkedButton.setGeometry(fill);
	}

	void CheckableIconButton::setChecked(bool checked)
	{
		if (_checked == checked)
		{
			return;
		}
		_checked = checked;
		_applyState();
		_stateProvider.trigger(_checked);
	}

	void CheckableIconButton::toggle()
	{
		setChecked(!_checked);
	}

	void CheckableIconButton::setUncheckedSpriteID(std::size_t spriteID)
	{
		_uncheckedButton.setIconSpriteID(spriteID);
	}

	void CheckableIconButton::setUncheckedSpriteID(const Vector2UInt &coordinates)
	{
		_uncheckedButton.setIconSpriteID(coordinates);
	}

	void CheckableIconButton::setCheckedSpriteID(std::size_t spriteID)
	{
		_checkedButton.setIconSpriteID(spriteID);
	}

	void CheckableIconButton::setCheckedSpriteID(const Vector2UInt &coordinates)
	{
		_checkedButton.setIconSpriteID(coordinates);
	}

	bool CheckableIconButton::isChecked() const noexcept
	{
		return _checked;
	}
	std::size_t CheckableIconButton::uncheckedSpriteID() const noexcept
	{
		return _uncheckedButton.iconSpriteID();
	}
	std::size_t CheckableIconButton::checkedSpriteID() const noexcept
	{
		return _checkedButton.iconSpriteID();
	}
	IconButton &CheckableIconButton::uncheckedButton() noexcept
	{
		return _uncheckedButton;
	}
	const IconButton &CheckableIconButton::uncheckedButton() const noexcept
	{
		return _uncheckedButton;
	}
	IconButton &CheckableIconButton::checkedButton() noexcept
	{
		return _checkedButton;
	}
	const IconButton &CheckableIconButton::checkedButton() const noexcept
	{
		return _checkedButton;
	}

	CheckableIconButton::StateContract CheckableIconButton::subscribeToState(StateCallback callback)
	{
		return _stateProvider.subscribe(std::move(callback));
	}

	CheckableIconButton::StateContract CheckableIconButton::addStateCallback(bool targetState, std::function<void()> callback)
	{
		return subscribeToState([targetState, callback = std::move(callback)](bool state) {
			if (state == targetState)
			{
				callback();
			}
		});
	}
}
