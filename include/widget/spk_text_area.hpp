// ────────────────────────────────────────────────────────────────
//  multiline_text_label.hpp
// ────────────────────────────────────────────────────────────────
#pragma once

#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <numeric>

#include "widget/spk_widget.hpp"
#include "widget/spk_linear_layout.hpp"
#include "structure/math/spk_vector2.hpp"
#include "structure/graphics/texture/spk_font.hpp"

#include "widget/spk_frame.hpp"
#include "widget/spk_text_label.hpp"
#include "widget/spk_spacer_widget.hpp"

namespace spk
{
    class TextArea : public Widget
    {
    private:
		spk::Frame _backgroundFrame;

		std::wstring _text;
		spk::Vector2UInt _cornerSize   = {10, 10};
		spk::Color _glyphColor   = spk::Color::white;
		spk::Color _outlineColor = spk::Color::black;
		spk::SafePointer<spk::Font> _font;
		spk::Font::Size _fontSize;
		spk::HorizontalAlignment _horizontalAlignment;
		spk::VerticalAlignment _verticalAlignment;

		spk::VerticalLayout _layout;
		spk::Vector2UInt _minimalSize;
		size_t _nbLine = 0;
		std::vector<std::unique_ptr<spk::TextLabel>> _labels;
		spk::SpacerWidget _topSpacer;
		spk::SpacerWidget _downSpacer;

        void _onGeometryChange() override;

		void _createNewLine();
		void _composeLayout();
		void _insertLine(const std::wstring& p_text);
		void _populateLabels(const spk::Vector2UInt &p_availableSize);

    public:
        TextArea(const std::wstring& p_name, spk::SafePointer<Widget> p_parent);

		void setLinePadding(size_t p_nbPixels);

		void setText(const std::wstring& p_text);
		void setTextAlignment(const spk::HorizontalAlignment& p_horizontalAlignment, const spk::VerticalAlignment& p_verticalAlignment);

		void setFont(spk::SafePointer<spk::Font> p_font);
		void setFontSize(const spk::Font::Size& p_size);
		void setFontColor(const spk::Color& p_glyphColor, const spk::Color& p_outlineColor);
		void setCornerSize(const spk::Vector2UInt& p_cornerSize);
		void setNineSlice(spk::SafePointer<const spk::SpriteSheet> p_spriteSheet);

		spk::Frame& backgroundFrame() {return (_backgroundFrame);}
		const spk::Frame& backgroundFrame() const {return (_backgroundFrame);}

		void setMinimalSize(const spk::Vector2UInt& p_minimalSize);
        Vector2UInt minimalSize() const override;
    };
}
