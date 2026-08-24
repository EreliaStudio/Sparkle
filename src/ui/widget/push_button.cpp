#include "ui/widget/push_button.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace spk
{
	PushButton::PushButton(std::string name, Widget *parent) :
		Widget(std::move(name), parent),
		_releasedBackground(this->name() + ".released-background", this),
		_pressedBackground(this->name() + ".pressed-background", this),
		_releasedLabel(this->name() + ".released-label", this),
		_pressedLabel(this->name() + ".pressed-label", this),
		_releasedIcon(this->name() + ".released-icon", this),
		_pressedIcon(this->name() + ".pressed-icon", this)
	{
		_releasedBackground.setZOrder(0.0f);
		_pressedBackground.setZOrder(0.0f);
		_releasedLabel.setZOrder(1.0f);
		_pressedLabel.setZOrder(1.0f);
		_releasedIcon.setZOrder(2.0f);
		_pressedIcon.setZOrder(2.0f);
		_applyVisualState();
		_updateSizeHint();
		activate();
	}

	Vector2UInt PushButton::_effectiveTextPadding() const
	{
		if (_textPadding.has_value())
		{
			return *_textPadding;
		}
		const Vector2Int corner = _releasedBackground.cornerSize();
		return {
			static_cast<unsigned int>(std::max(corner.x, 0)),
			static_cast<unsigned int>(std::max(corner.y, 0))};
	}

	Vector2UInt PushButton::_effectiveIconPadding() const
	{
		if (_iconPadding.has_value())
		{
			return *_iconPadding;
		}
		const Vector2Int corner = _releasedBackground.cornerSize();
		return {
			static_cast<unsigned int>(std::max(corner.x, 0)),
			static_cast<unsigned int>(std::max(corner.y, 0))};
	}

	Vector2UInt PushButton::_naturalIconSize() const
	{
		const Texture *texture = _releasedIcon.texture();
		if (!_hasIcon || texture == nullptr)
		{
			return {0, 0};
		}
		const Texture::Section &section = _releasedIcon.section();
		return {
			static_cast<unsigned int>(std::lround(static_cast<float>(texture->size().x) * std::max(section.size.x, 0.0f))),
			static_cast<unsigned int>(std::lround(static_cast<float>(texture->size().y) * std::max(section.size.y, 0.0f)))};
	}

	void PushButton::_applyVisualState()
	{
		if (_pressed)
		{
			_releasedBackground.deactivate();
			_releasedLabel.deactivate();
			_releasedIcon.deactivate();
			_flat ? _pressedBackground.deactivate() : _pressedBackground.activate();
			_pressedLabel.activate();
			_hasIcon ? _pressedIcon.activate() : _pressedIcon.deactivate();
		}
		else
		{
			_pressedBackground.deactivate();
			_pressedLabel.deactivate();
			_pressedIcon.deactivate();
			_flat ? _releasedBackground.deactivate() : _releasedBackground.activate();
			_releasedLabel.activate();
			_hasIcon ? _releasedIcon.activate() : _releasedIcon.deactivate();
		}
	}

	void PushButton::_updateTextGeometry()
	{
		const Vector2UInt padding = _effectiveTextPadding();
		const unsigned int horizontalPadding = std::min(padding.x, geometry().width / 2);
		const unsigned int verticalPadding = std::min(padding.y, geometry().height / 2);
		const Rect2D textGeometry{
			.anchor = {static_cast<int>(horizontalPadding), static_cast<int>(verticalPadding)},
			.size = {geometry().width - 2 * horizontalPadding, geometry().height - 2 * verticalPadding}};
		_releasedLabel.setGeometry(textGeometry);
		_pressedLabel.setGeometry(textGeometry);
	}

	void PushButton::_updateIconGeometry()
	{
		const Vector2UInt padding = _effectiveIconPadding();
		const unsigned int horizontalPadding = std::min(padding.x, geometry().width / 2);
		const unsigned int verticalPadding = std::min(padding.y, geometry().height / 2);
		Rect2D iconGeometry{
			.anchor = {static_cast<int>(horizontalPadding), static_cast<int>(verticalPadding)},
			.size = {geometry().width - 2 * horizontalPadding, geometry().height - 2 * verticalPadding}};

		if (_iconSize.has_value())
		{
			const Vector2UInt size{
				std::min(_iconSize->x, iconGeometry.width),
				std::min(_iconSize->y, iconGeometry.height)};
			iconGeometry.anchor += Vector2Int{
				static_cast<int>((iconGeometry.width - size.x) / 2),
				static_cast<int>((iconGeometry.height - size.y) / 2)};
			iconGeometry.size = size;
		}

		_releasedIcon.setGeometry(iconGeometry);
		_pressedIcon.setGeometry(iconGeometry);
	}

	void PushButton::_updateSizeHint()
	{
		_updateTextGeometry();
		_updateIconGeometry();
		const Vector2UInt textPadding = _effectiveTextPadding();
		Vector2 intrinsic{
			std::max(_releasedLabel.minimalSize().x, _pressedLabel.minimalSize().x) + 2.0f * static_cast<float>(textPadding.x),
			std::max(_releasedLabel.minimalSize().y, _pressedLabel.minimalSize().y) + 2.0f * static_cast<float>(textPadding.y)};

		const Vector2Int corner = _releasedBackground.cornerSize();
		intrinsic.x = std::max(intrinsic.x, static_cast<float>(std::max(corner.x, 0) * 2));
		intrinsic.y = std::max(intrinsic.y, static_cast<float>(std::max(corner.y, 0) * 2));

		if (_hasIcon)
		{
			const Vector2UInt size = _iconSize.value_or(_naturalIconSize());
			const Vector2UInt padding = _effectiveIconPadding();
			intrinsic.x = std::max(intrinsic.x, static_cast<float>(size.x + 2 * padding.x));
			intrinsic.y = std::max(intrinsic.y, static_cast<float>(size.y + 2 * padding.y));
		}

		SizeHint hint = sizeHint();
		hint.minimal = intrinsic;
		hint.preferred = intrinsic;
		setSizeHint(hint);
	}

	void PushButton::_onGeometryChange()
	{
		const Rect2D fill{Vector2Int{0, 0}, geometry().size};
		_releasedBackground.setGeometry(fill);
		_pressedBackground.setGeometry(fill);
		_updateTextGeometry();
		_updateIconGeometry();
	}

	void PushButton::_onMouseLeftEvent(MouseLeftEvent &)
	{
		_hovered = false;
	}

	void PushButton::_onMouseMovedEvent(MouseMovedEvent &event)
	{
		_hovered = viewRegion().viewport.contains(event.device.position);
	}

	void PushButton::_onMouseButtonPressedEvent(MouseButtonPressedEvent &event)
	{
		if (event.record.button != Mouse::Button::Left || !viewRegion().viewport.contains(event.device.position))
		{
			return;
		}
		_hovered = true;
		_pressed = true;
		_applyVisualState();
		event.consumed = true;
	}

	void PushButton::_onMouseButtonReleasedEvent(MouseButtonReleasedEvent &event)
	{
		if (event.record.button != Mouse::Button::Left || !_pressed)
		{
			return;
		}

		_pressed = false;
		_hovered = viewRegion().viewport.contains(event.device.position);
		_applyVisualState();
		if (_hovered)
		{
			_clickProvider.trigger();
			event.consumed = true;
		}
	}

	void PushButton::_onMouseButtonDoubleClickedEvent(MouseButtonDoubleClickedEvent &event)
	{
		if (event.record.button != Mouse::Button::Left || !viewRegion().viewport.contains(event.device.position))
		{
			return;
		}
		_hovered = true;
		_pressed = true;
		_applyVisualState();
		event.consumed = true;
	}

	PushButton::ClickContract PushButton::subscribeToClick(ClickCallback callback)
	{
		return _clickProvider.subscribe(std::move(callback));
	}

	void PushButton::setText(const Font::Text &text)
	{
		_releasedLabel.setText(text);
		_pressedLabel.setText(text);
		_updateSizeHint();
	}

	void PushButton::setText(std::string_view text)
	{
		setText(Font::textFromUTF8(text));
	}

	void PushButton::setTextPadding(const Vector2UInt &padding)
	{
		if (_textPadding == padding)
		{
			return;
		}
		_textPadding = padding;
		_updateTextGeometry();
		_updateSizeHint();
	}

	void PushButton::resetTextPadding()
	{
		if (!_textPadding.has_value())
		{
			return;
		}
		_textPadding.reset();
		_updateTextGeometry();
		_updateSizeHint();
	}

	void PushButton::setAlignment(HorizontalAlignment horizontal, VerticalAlignment vertical)
	{
		_releasedLabel.setAlignment(horizontal, vertical);
		_pressedLabel.setAlignment(horizontal, vertical);
	}

	void PushButton::setIcon(const Texture *texture, const Texture::Section &section)
	{
		_releasedIcon.setTexture(texture);
		_pressedIcon.setTexture(texture);
		_releasedIcon.setSection(section);
		_pressedIcon.setSection(section);
		_hasIcon = true;
		_applyVisualState();
		_updateIconGeometry();
		_updateSizeHint();
	}

	void PushButton::setIcon(const SpriteSheet *spriteSheet, std::size_t spriteID)
	{
		if (spriteSheet == nullptr)
		{
			throw std::invalid_argument("PushButton sprite sheet cannot be null");
		}
		setIcon(spriteSheet, spriteSheet->sprite(spriteID));
	}

	void PushButton::setIcon(const SpriteSheet *spriteSheet, const Vector2UInt &coordinates)
	{
		if (spriteSheet == nullptr)
		{
			throw std::invalid_argument("PushButton sprite sheet cannot be null");
		}
		setIcon(spriteSheet, spriteSheet->sprite(coordinates));
	}

	void PushButton::setIconSize(const Vector2UInt &size)
	{
		_iconSize = size;
		_updateIconGeometry();
		_updateSizeHint();
	}

	void PushButton::resetIconSize()
	{
		_iconSize.reset();
		_updateIconGeometry();
		_updateSizeHint();
	}

	void PushButton::setIconPadding(const Vector2UInt &padding)
	{
		_iconPadding = padding;
		_updateIconGeometry();
		_updateSizeHint();
	}

	void PushButton::resetIconPadding()
	{
		_iconPadding.reset();
		_updateIconGeometry();
		_updateSizeHint();
	}

	void PushButton::removeIcon()
	{
		_hasIcon = false;
		_applyVisualState();
		_updateSizeHint();
	}

	void PushButton::setFlat(bool flat)
	{
		_flat = flat;
		_applyVisualState();
	}

	bool PushButton::hasIcon() const noexcept
	{
		return _hasIcon;
	}
	bool PushButton::isHovered() const noexcept
	{
		return _hovered;
	}
	bool PushButton::isPressed() const noexcept
	{
		return _pressed;
	}
	bool PushButton::isFlat() const noexcept
	{
		return _flat;
	}
	const std::optional<Vector2UInt> &PushButton::textPadding() const noexcept
	{
		return _textPadding;
	}
	const std::optional<Vector2UInt> &PushButton::iconSize() const noexcept
	{
		return _iconSize;
	}
	const std::optional<Vector2UInt> &PushButton::iconPadding() const noexcept
	{
		return _iconPadding;
	}
	Panel &PushButton::releasedBackground() noexcept
	{
		return _releasedBackground;
	}
	const Panel &PushButton::releasedBackground() const noexcept
	{
		return _releasedBackground;
	}
	Panel &PushButton::pressedBackground() noexcept
	{
		return _pressedBackground;
	}
	const Panel &PushButton::pressedBackground() const noexcept
	{
		return _pressedBackground;
	}
	TextLabel &PushButton::releasedLabel() noexcept
	{
		return _releasedLabel;
	}
	const TextLabel &PushButton::releasedLabel() const noexcept
	{
		return _releasedLabel;
	}
	TextLabel &PushButton::pressedLabel() noexcept
	{
		return _pressedLabel;
	}
	const TextLabel &PushButton::pressedLabel() const noexcept
	{
		return _pressedLabel;
	}
	ImageLabel &PushButton::releasedIcon() noexcept
	{
		return _releasedIcon;
	}
	const ImageLabel &PushButton::releasedIcon() const noexcept
	{
		return _releasedIcon;
	}
	ImageLabel &PushButton::pressedIcon() noexcept
	{
		return _pressedIcon;
	}
	const ImageLabel &PushButton::pressedIcon() const noexcept
	{
		return _pressedIcon;
	}
}
