#pragma once

#include <memory>
#include <stdexcept>

#include "structure/design_pattern/spk_activable_object.hpp"
#include "structure/design_pattern/spk_contract_provider.hpp"
#include "structure/design_pattern/spk_inherence_object.hpp"
#include "structure/graphics/renderer/spk_color_renderer.hpp"
#include "structure/graphics/renderer/spk_font_renderer.hpp"
#include "structure/graphics/renderer/spk_nine_slice_renderer.hpp"
#include "structure/graphics/spk_geometry_2D.hpp"
#include "structure/graphics/spk_viewport.hpp"
#include "structure/graphics/texture/spk_sprite_sheet.hpp"
#include "structure/math/spk_vector2.hpp"
#include "structure/spk_safe_pointer.hpp"
#include "structure/system/event/spk_event.hpp"
#include "widget/spk_widget.hpp"

namespace spk
{
	class PushButton : public Widget
	{
	public:
		enum class State
		{
			Pressed,
			Released,
			Hovered
		};

		static spk::SafePointer<const spk::SpriteSheet> defaultHoverNineSlice();

	private:
		static spk::SpriteSheet _defaultHoverNineSlice;

		inline State _currentVisualState() const
		{
			if (_isPressed)
			{
				return State::Pressed;
			}
			if (_isHovered)
			{
				return State::Hovered;
			}
			return State::Released;
		}

		bool _isPressed;
		bool _isHovered;

		template <typename TType>
		struct Data
		{
			TType released;
			TType hovered;
			TType pressed;

			TType &operator[](const State &p_state)
			{
				switch (p_state)
				{
				case State::Released:
					return (released);
				case State::Pressed:
					return (pressed);
				default:
					return (hovered);
				}
			}

			const TType &operator[](const State &p_state) const
			{
				switch (p_state)
				{
				case State::Released:
					return (released);
				case State::Pressed:
					return (pressed);
				default:
					return (hovered);
				}
			}
		};

		ContractProvider _onClickProvider;

		Data<FontRenderer> _fontRenderer;
		Data<FontRenderer::Contract> _fontRendererContract;
		Data<TextureRenderer> _iconRenderer;
		Data<NineSliceRenderer> _nineSliceRenderer;

		Data<spk::Vector2UInt> _cornerSize;

		Data<std::wstring> _text;
		Data<spk::SpriteSheet::Sprite> _icon;
		Data<spk::VerticalAlignment> _verticalAlignment;
		Data<spk::HorizontalAlignment> _horizontalAlignment;

		spk::Vector2Int _pressedOffset;

	private:
		virtual void _onGeometryChange() override;

		void _onPaintEvent(spk::PaintEvent &p_event);
		void _onMouseEvent(spk::MouseEvent &p_event);

	public:
		PushButton(const std::wstring &p_name, const spk::SafePointer<spk::Widget> &p_parent);
		~PushButton() = default;

		spk::Vector2UInt computeTextSize() const;
		spk::Vector2UInt computeExpectedTextSize(const spk::Font::Size &p_textSize) const;

		spk::Vector2UInt minimalSize() const override;

		ContractProvider::Contract subscribe(const ContractProvider::Job &p_job);

		void setFont(const spk::SafePointer<spk::Font> &p_font);
		void setFont(const spk::SafePointer<spk::Font> &p_font, const State &p_state);

		void setText(const std::wstring &p_text);
		void setText(const std::wstring &p_text, const State &p_state);

		void setFontSize(const spk::Font::Size &p_fontSize);
		void setFontSize(const spk::Font::Size &p_fontSize, const State &p_state);

		void setTextColor(const spk::Color &p_glyphColor, const spk::Color &p_outlineColor);
		void setTextColor(const spk::Color &p_glyphColor, const spk::Color &p_outlineColor, const State &p_state);

		void setTextAlignment(const spk::HorizontalAlignment &p_horizontalAlignment, const spk::VerticalAlignment &p_verticalAlignment);
		void setTextAlignment(const spk::HorizontalAlignment &p_horizontalAlignment, const spk::VerticalAlignment &p_verticalAlignment,
							  const State &p_state);

		void setIconset(spk::SafePointer<const SpriteSheet> p_iconset);
		void setIconset(spk::SafePointer<const SpriteSheet> p_iconset, const State &p_state);

		void setIcon(const spk::SpriteSheet::Sprite &p_icon);
		void setIcon(const spk::SpriteSheet::Sprite &p_icon, const State &p_state);

		void setCornerSize(const spk::Vector2Int &p_cornerSize);
		void setCornerSize(const spk::Vector2Int &p_cornerSize, const State &p_state);

		void setNineSlice(const SafePointer<const SpriteSheet> &p_spriteSheet);
		void setNineSlice(const SafePointer<const SpriteSheet> &p_spriteSheet, const State &p_state);

		void setPressedOffset(const spk::Vector2Int &p_offset);

		const spk::Vector2Int &pressedOffset() const;

		const spk::SafePointer<const spk::SpriteSheet> &spriteSheet(State p_state = State::Released) const;
		const spk::SafePointer<const spk::SpriteSheet> &iconset(State p_state = State::Released) const;
		const spk::SafePointer<spk::Font> &font(State p_state = State::Released) const;

		const spk::Vector2UInt &cornerSize(State p_state = State::Released) const;
		const std::wstring &text(State p_state = State::Released) const;
		const spk::Font::Size &fontSize(State p_state = State::Released) const;
		const spk::SpriteSheet::Sprite &icon(State p_state = State::Released) const;
		const spk::VerticalAlignment &verticalAlignment(State p_state = State::Released) const;
		const spk::HorizontalAlignment &horizontalAlignment(State p_state = State::Released) const;
	};
}
