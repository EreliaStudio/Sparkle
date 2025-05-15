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
    class MultilineTextLabel : public Widget
    {
    private:
		spk::Frame _backgroundFrame;

		std::wstring _text;
		spk::SafePointer<spk::Font> _font;
		spk::Font::Size _fontSize;
		spk::HorizontalAlignment _horizontalAlignment;
		spk::VerticalAlignment _verticalAlignment;

		spk::VerticalLayout _layout;
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
        MultilineTextLabel(const std::wstring& p_name, spk::SafePointer<Widget> p_parent);

		void setText(const std::wstring& p_text);
		void setTextAlignment(const spk::HorizontalAlignment& p_horizontalAlignment, const spk::VerticalAlignment& p_verticalAlignment);

		spk::Frame& backgroundFrame() {return (_backgroundFrame);}
		const spk::Frame& backgroundFrame() const {return (_backgroundFrame);}

        Vector2UInt minimalSize() const override;
    };
}
