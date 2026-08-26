#include "ui/widget/icon_button.hpp"

#include <stdexcept>
#include <utility>

namespace spk
{
	IconButton::IconButton(std::string name, Widget *parent) :
		PushButton(std::move(name), parent)
	{
		applyStyle(defaultStyle);
		setIconSize({22, 22});
		setIconPadding({6, 6});
	}

	IconButton::IconButton(std::string name, const SpriteSheet *iconset, std::size_t spriteID, Widget *parent) :
		IconButton(std::move(name), parent)
	{
		_iconSpriteID = spriteID;
		setIconset(iconset);
	}

	void IconButton::applyStyle(const Style &style)
	{
		PushButton::applyStyle(style);
		if (style.iconset != nullptr)
		{
			setIconset(style.iconset.get());
		}
	}

	void IconButton::_refreshIcon()
	{
		if (_iconset != nullptr)
		{
			setIcon(_iconset, _iconSpriteID);
		}
	}

	void IconButton::setIconset(const SpriteSheet *iconset)
	{
		if (iconset == nullptr)
		{
			throw std::invalid_argument("IconButton iconset cannot be null");
		}
		_iconset = iconset;
		_refreshIcon();
	}

	void IconButton::setIconSpriteID(std::size_t spriteID)
	{
		_iconSpriteID = spriteID;
		_refreshIcon();
	}

	void IconButton::setIconSpriteID(const Vector2UInt &coordinates)
	{
		if (_iconset == nullptr)
		{
			throw std::logic_error("IconButton requires an iconset before resolving sprite coordinates");
		}
		setIconSpriteID(_iconset->spriteID(coordinates));
	}

	const SpriteSheet *IconButton::iconset() const noexcept
	{
		return _iconset;
	}

	std::size_t IconButton::iconSpriteID() const noexcept
	{
		return _iconSpriteID;
	}
}
