#pragma once

#include "structure/spk_safe_pointer.hpp"

#include "structure/design_pattern/spk_inherence_object.hpp"
#include "structure/design_pattern/spk_activable_object.hpp"

#include "structure/graphics/spk_geometry_2D.hpp"
#include "structure/graphics/spk_viewport.hpp"

#include "structure/system/event/spk_event.hpp"

#include "widget/spk_widget.hpp"

#include "structure/graphics/renderer/spk_nine_slice_renderer.hpp"
#include "structure/graphics/renderer/spk_font_renderer.hpp"
#include "structure/graphics/renderer/spk_color_renderer.hpp"
#include "structure/graphics/texture/spk_sprite_sheet.hpp"
#include "structure/math/spk_vector2.hpp"

#include "structure/design_pattern/spk_contract_provider.hpp"

#include <memory>
#include <stdexcept>

namespace spk
{ 
    class PushButton : public Widget
    {
    private:
        bool _isPressed;
		
		ContractProvider _onClickProvider;

		FontRenderer _releasedFontRenderer;
		FontRenderer _pressedFontRenderer;
        NineSliceRenderer _releasedRenderer;
        NineSliceRenderer _pressedRenderer;

        spk::Vector2Int _pressedOffset;

        spk::Vector2Int _releasedCornerSize;
        spk::Vector2Int _pressedCornerSize;

		std::wstring _releasedText;
		std::wstring _pressedText;

		spk::VerticalAlignment _releasedVerticalAlignment;
		spk::VerticalAlignment _pressedVerticalAlignment;

		spk::HorizontalAlignment _releasedHorizontalAlignment;
		spk::HorizontalAlignment _pressedHorizontalAlignment;

    private:
        virtual void _onGeometryChange() override;

    	void _onPaintEvent(spk::PaintEvent& p_event);
    	void _onMouseEvent(spk::MouseEvent& p_event);

    public:
        PushButton(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent);

		ContractProvider::Contract subscribe(const ContractProvider::Job& p_job);

		void setFont(const spk::SafePointer<spk::Font>& p_font);
		void setFont(const spk::SafePointer<spk::Font>& p_releasedFont, const spk::SafePointer<spk::Font>& p_pressedFont);
		void setText(const std::wstring& p_text);
		void setText(const std::wstring& p_releasedText, const std::wstring& p_pressedText);
		void setTextSize(const spk::Font::Size& p_textSize);
		void setTextSize(const spk::Font::Size& p_releasedTextSize, const spk::Font::Size& p_pressedTextSize);
		void setTextColor(const spk::Color& p_glyphColor, const spk::Color& p_outlineColor);
		void setTextColor(
			const spk::Color& p_releasedGlyphColor, const spk::Color& p_releasedOutlineColor,
			const spk::Color& p_pressedGlyphColor, const spk::Color& p_pressedOutlineColor
		);
		void setTextAlignment(const spk::HorizontalAlignment& p_horizontalAlignment, const spk::VerticalAlignment& p_verticalAlignment);
		void setTextAlignment(
			const spk::HorizontalAlignment& p_releasedHorizontalAlignment, const spk::VerticalAlignment& p_releasedVerticalAlignment, 
			const spk::HorizontalAlignment& p_pressedHorizontalAlignment, const spk::VerticalAlignment& p_pressedVerticalAlignment
		);

		void setCornerSize(const spk::Vector2Int& p_cornerSize);
		void setCornerSize(const spk::Vector2Int& p_releasedCornerSize, const spk::Vector2Int& p_pressedCornerSize);

		void setSpriteSheet(const SafePointer<SpriteSheet>& p_spriteSheet);
		void setSpriteSheet(const SafePointer<SpriteSheet>& p_releasedSpriteSheet, const SafePointer<SpriteSheet>& p_pressedSpriteSheet);

        void setPressedOffset(const spk::Vector2Int& p_offset);

        const spk::Vector2Int& pressedOffset() const;
    };
}
