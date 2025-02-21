#pragma once

#include <string>
#include "widget/spk_widget.hpp"

#include "structure/graphics/renderer/spk_nine_slice_renderer.hpp"
#include "structure/graphics/renderer/spk_font_renderer.hpp"
#include "structure/graphics/renderer/spk_color_renderer.hpp"

namespace spk
{
	class TextEdit : public spk::Widget
	{
	private:
		bool _isEditEnable = true;
		bool _renderCursor = true;
		bool _needLowerCursorUpdate = true;
		bool _needHigherCursorUpdate = true;
		size_t _cursor = 0;
		size_t _lowerCursor;
		size_t _higherCursor;
		std::wstring _text;
		std::wstring _placeholder;

		spk::Vector2Int _cornerSize = {2, 2};
		spk::HorizontalAlignment _horizontalAlignment = spk::HorizontalAlignment::Left;
		spk::VerticalAlignment _verticalAlignment = spk::VerticalAlignment::Centered;

		spk::NineSliceRenderer _backgroundRenderer;
		spk::ColorRenderer _cursorRenderer;
		spk::FontRenderer _fontRenderer;

		void _computeCursorsValues();
		void _onGeometryChange() override;
		void _onPaintEvent(spk::PaintEvent& p_event) override;
		void _onUpdateEvent(spk::UpdateEvent& p_event) override;
		void _onMouseEvent(spk::MouseEvent& p_event) override;
		void _onKeyboardEvent(spk::KeyboardEvent& p_event) override;

	public:
		TextEdit(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent);

		void setNineSlice(const spk::SafePointer<spk::SpriteSheet>& p_spriteSheet);
		void setCornerSize(const spk::Vector2Int& p_cornerSize);
		void setFont(const spk::SafePointer<spk::Font>& p_font);
		void setFontSize(const spk::Font::Size& p_textSize);
		void setTextColor(const spk::Color& p_glyphColor, const spk::Color& p_outlineColor);
		void setTextAlignment(const spk::HorizontalAlignment& p_horizontalAlignment, const spk::VerticalAlignment& p_verticalAlignment);
		void setText(const std::wstring& p_text);
		void setPlaceholder(const std::wstring& p_placeholder);

		void disableEdit();
		void enableEdit();

		bool isEditEnable() const;
		bool hasText() const;
		const std::wstring& text() const;

		const spk::Vector2Int& cornerSize() const;
	};
}