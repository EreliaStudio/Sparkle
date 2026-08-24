#pragma once

#include <functional>
#include <optional>
#include <string>

#include "design_pattern/contract_provider.hpp"
#include "ui/widget/image_label.hpp"
#include "ui/widget/panel.hpp"
#include "ui/widget/text_label.hpp"

namespace spk
{
	class PushButton : public Widget
	{
	public:
		using ClickProvider = ContractProvider<>;
		using ClickCallback = ClickProvider::callback_type;
		using ClickContract = ClickProvider::Contract;

	private:
		Panel _releasedBackground;
		Panel _pressedBackground;
		TextLabel _releasedLabel;
		TextLabel _pressedLabel;
		ImageLabel _releasedIcon;
		ImageLabel _pressedIcon;

		bool _hovered = false;
		bool _pressed = false;
		bool _hasIcon = false;
		bool _flat = false;
		std::optional<Vector2UInt> _iconSize;
		std::optional<Vector2UInt> _iconPadding;
		ClickProvider _clickProvider;

		[[nodiscard]] Vector2UInt _effectiveIconPadding() const;
		[[nodiscard]] Vector2UInt _naturalIconSize() const;
		void _applyVisualState();
		void _updateIconGeometry();
		void _updateSizeHint() override;
		void _onGeometryChange() override;
		void _onMouseLeftEvent(MouseLeftEvent &event) override;
		void _onMouseMovedEvent(MouseMovedEvent &event) override;
		void _onMouseButtonPressedEvent(MouseButtonPressedEvent &event) override;
		void _onMouseButtonReleasedEvent(MouseButtonReleasedEvent &event) override;

	public:
		explicit PushButton(std::string name, Widget *parent = nullptr);

		[[nodiscard]] ClickContract subscribeToClick(ClickCallback callback);

		void setText(const Font::Text &text);
		void setText(std::string_view text);
		void setAlignment(HorizontalAlignment horizontal, VerticalAlignment vertical);
		void setIcon(const Texture *texture, const Texture::Section &section = Texture::Section::whole);
		void setIcon(const SpriteSheet *spriteSheet, std::size_t spriteID);
		void setIcon(const SpriteSheet *spriteSheet, const Vector2UInt &coordinates);
		void setIconSize(const Vector2UInt &size);
		void resetIconSize();
		void setIconPadding(const Vector2UInt &padding);
		void resetIconPadding();
		void removeIcon();
		void setFlat(bool flat);

		[[nodiscard]] bool hasIcon() const noexcept;
		[[nodiscard]] bool isHovered() const noexcept;
		[[nodiscard]] bool isPressed() const noexcept;
		[[nodiscard]] bool isFlat() const noexcept;
		[[nodiscard]] const std::optional<Vector2UInt> &iconSize() const noexcept;
		[[nodiscard]] const std::optional<Vector2UInt> &iconPadding() const noexcept;

		[[nodiscard]] Panel &releasedBackground() noexcept;
		[[nodiscard]] const Panel &releasedBackground() const noexcept;
		[[nodiscard]] Panel &pressedBackground() noexcept;
		[[nodiscard]] const Panel &pressedBackground() const noexcept;
		[[nodiscard]] TextLabel &releasedLabel() noexcept;
		[[nodiscard]] const TextLabel &releasedLabel() const noexcept;
		[[nodiscard]] TextLabel &pressedLabel() noexcept;
		[[nodiscard]] const TextLabel &pressedLabel() const noexcept;
		[[nodiscard]] ImageLabel &releasedIcon() noexcept;
		[[nodiscard]] const ImageLabel &releasedIcon() const noexcept;
		[[nodiscard]] ImageLabel &pressedIcon() noexcept;
		[[nodiscard]] const ImageLabel &pressedIcon() const noexcept;
	};
}
