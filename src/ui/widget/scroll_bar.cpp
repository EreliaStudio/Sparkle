#include "ui/widget/scroll_bar.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace spk
{
	ScrollBar::ScrollBar(std::string name, Widget *parent) :
		Widget(std::move(name), parent),
		_negativeButton(this->name() + ".negative", this),
		_slider(this->name() + ".slider", this),
		_positiveButton(this->name() + ".positive", this)
	{
		applyStyle(defaultStyle);
		_updateIcons();
		_negativeClickContract = _negativeButton.subscribeToClick([this]() {
			_slider.setRatio(_slider.ratio() - _step);
		});
		_sliderEditionContract = _slider.subscribeToEdition([this](float ratio) {
			_editionProvider.trigger(ratio);
		});
		_positiveClickContract = _positiveButton.subscribeToClick([this]() {
			_slider.setRatio(_slider.ratio() + _step);
		});
		_updateSizeHint();
		activate();
	}

	ScrollBar::ScrollBar(std::string name, const SpriteSheet *iconset, Orientation orientation, Widget *parent) :
		ScrollBar(std::move(name), parent)
	{
		setIconset(iconset);
		setOrientation(orientation);
	}

	void ScrollBar::applyStyle(const Style &style)
	{
		_negativeButton.applyStyle(style);
		_slider.applyStyle(style);
		_positiveButton.applyStyle(style);
		if (style.iconset != nullptr)
		{
			setIconset(style.iconset.get());
		}
		_negativeButton.setIconSize(style.scrollBarButtonIconSize);
		_positiveButton.setIconSize(style.scrollBarButtonIconSize);
		_updateSizeHint();
	}

	void ScrollBar::_updateIcons()
	{
		if (_orientation == Orientation::Horizontal)
		{
			_negativeButton.setIconSpriteID(_arrowSpriteIDs.left);
			_positiveButton.setIconSpriteID(_arrowSpriteIDs.right);
		}
		else
		{
			_negativeButton.setIconSpriteID(_arrowSpriteIDs.up);
			_positiveButton.setIconSpriteID(_arrowSpriteIDs.down);
		}
	}

	void ScrollBar::_updateSizeHint()
	{
		const Vector2 &negativeMinimum = _negativeButton.minimalSize();
		const Vector2 &sliderMinimum = _slider.minimalSize();
		const Vector2 &positiveMinimum = _positiveButton.minimalSize();
		const Vector2 &negativePreferred = _negativeButton.preferredSize();
		const Vector2 &sliderPreferred = _slider.preferredSize();
		const Vector2 &positivePreferred = _positiveButton.preferredSize();

		SizeHint hint = sizeHint();
		if (_orientation == Orientation::Horizontal)
		{
			hint.minimal = {negativeMinimum.x + sliderMinimum.x + positiveMinimum.x, std::max({negativeMinimum.y, sliderMinimum.y, positiveMinimum.y})};
			hint.preferred = {negativePreferred.x + sliderPreferred.x + positivePreferred.x, std::max({negativePreferred.y, sliderPreferred.y, positivePreferred.y})};
		}
		else
		{
			hint.minimal = {std::max({negativeMinimum.x, sliderMinimum.x, positiveMinimum.x}), negativeMinimum.y + sliderMinimum.y + positiveMinimum.y};
			hint.preferred = {std::max({negativePreferred.x, sliderPreferred.x, positivePreferred.x}), negativePreferred.y + sliderPreferred.y + positivePreferred.y};
		}
		setSizeHint(hint);
	}

	void ScrollBar::_onGeometryChange()
	{
		if (_orientation == Orientation::Horizontal)
		{
			const unsigned int buttonLength = std::min(geometry().height, geometry().width / 2);
			_negativeButton.setGeometry({Vector2Int{0, 0}, Vector2UInt{buttonLength, geometry().height}});
			_slider.setGeometry({Vector2Int{static_cast<int>(buttonLength), 0}, Vector2UInt{geometry().width - 2 * buttonLength, geometry().height}});
			_positiveButton.setGeometry({Vector2Int{static_cast<int>(geometry().width - buttonLength), 0}, Vector2UInt{buttonLength, geometry().height}});
		}
		else
		{
			const unsigned int buttonLength = std::min(geometry().width, geometry().height / 2);
			_negativeButton.setGeometry({Vector2Int{0, 0}, Vector2UInt{geometry().width, buttonLength}});
			_slider.setGeometry({Vector2Int{0, static_cast<int>(buttonLength)}, Vector2UInt{geometry().width, geometry().height - 2 * buttonLength}});
			_positiveButton.setGeometry({Vector2Int{0, static_cast<int>(geometry().height - buttonLength)}, Vector2UInt{geometry().width, buttonLength}});
		}
	}

	ScrollBar::EditionContract ScrollBar::subscribeToEdition(EditionCallback callback)
	{
		return _editionProvider.subscribe(std::move(callback));
	}

	void ScrollBar::setOrientation(Orientation orientation)
	{
		if (_orientation == orientation)
		{
			return;
		}
		_orientation = orientation;
		_slider.setOrientation(orientation);
		_updateIcons();
		_updateSizeHint();
		_onGeometryChange();
	}

	void ScrollBar::setStep(float step)
	{
		if (!std::isfinite(step) || step <= 0.0f || step > 1.0f)
		{
			throw std::invalid_argument("ScrollBar step must be in (0, 1]");
		}
		_step = step;
	}

	void ScrollBar::setScale(float scale)
	{
		_slider.setScale(scale);
	}

	void ScrollBar::setRatio(float ratio)
	{
		_slider.setRatio(ratio);
	}

	void ScrollBar::setRange(float minimum, float maximum)
	{
		_slider.setRange(minimum, maximum);
	}

	void ScrollBar::setValue(float value)
	{
		_slider.setValue(value);
	}

	void ScrollBar::setIconset(const SpriteSheet *iconset)
	{
		if (iconset == nullptr)
		{
			throw std::invalid_argument("ScrollBar iconset cannot be null");
		}
		_iconset = iconset;
		_updateIcons();
		_negativeButton.setIconset(iconset);
		_positiveButton.setIconset(iconset);
	}

	void ScrollBar::setArrowSpriteIDs(const ArrowSpriteIDs &spriteIDs)
	{
		_arrowSpriteIDs = spriteIDs;
		_updateIcons();
	}

	Orientation ScrollBar::orientation() const noexcept
	{
		return _orientation;
	}

	float ScrollBar::step() const noexcept
	{
		return _step;
	}

	float ScrollBar::scale() const noexcept
	{
		return _slider.scale();
	}

	float ScrollBar::ratio() const noexcept
	{
		return _slider.ratio();
	}

	float ScrollBar::value() const noexcept
	{
		return _slider.value();
	}

	const SliderBar::Range &ScrollBar::range() const noexcept
	{
		return _slider.range();
	}

	const SpriteSheet *ScrollBar::iconset() const noexcept
	{
		return _iconset;
	}

	const ScrollBar::ArrowSpriteIDs &ScrollBar::arrowSpriteIDs() const noexcept
	{
		return _arrowSpriteIDs;
	}

	IconButton &ScrollBar::negativeButton() noexcept
	{
		return _negativeButton;
	}

	const IconButton &ScrollBar::negativeButton() const noexcept
	{
		return _negativeButton;
	}

	SliderBar &ScrollBar::slider() noexcept
	{
		return _slider;
	}

	const SliderBar &ScrollBar::slider() const noexcept
	{
		return _slider;
	}

	IconButton &ScrollBar::positiveButton() noexcept
	{
		return _positiveButton;
	}

	const IconButton &ScrollBar::positiveButton() const noexcept
	{
		return _positiveButton;
	}
}
