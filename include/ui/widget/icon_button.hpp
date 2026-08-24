#pragma once

#include <cstddef>
#include <string>

#include "ui/widget/push_button.hpp"

namespace spk
{
	class IconButton : public PushButton
	{
	private:
		const SpriteSheet *_iconset = nullptr;
		std::size_t _iconSpriteID = 0;

		void _refreshIcon();

	public:
		explicit IconButton(std::string name, Widget *parent = nullptr);
		IconButton(std::string name, const SpriteSheet *iconset, std::size_t spriteID = 0, Widget *parent = nullptr);

		void setIconset(const SpriteSheet *iconset);
		void setIconSpriteID(std::size_t spriteID);
		void setIconSpriteID(const Vector2UInt &coordinates);

		[[nodiscard]] const SpriteSheet *iconset() const noexcept;
		[[nodiscard]] std::size_t iconSpriteID() const noexcept;
	};
}
