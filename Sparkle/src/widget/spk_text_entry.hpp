#pragma once

#include "math/spk_vector2.hpp"
#include "spk_basic_functions.hpp"
#include "widget/components/spk_nine_sliced.hpp"
#include "widget/components/spk_text_label.hpp"
#include "widget/components/spk_colored_box.hpp"
#include "widget/spk_widget.hpp"
#include "input/spk_input.hpp"

namespace spk
{
	class TextEntry : public Widget
    {
    private:
        spk::WidgetComponent::NineSlicedBox _box;
        spk::WidgetComponent::ColoredBox _cursorBox;
        spk::WidgetComponent::TextLabel _label;

        std::vector<spk::Input> _inputs;

		bool _isSelected = false;
        size_t _currentCursorPosition = 0;
        size_t _openingCursorPosition = 0;
        size_t _closingCursorPosition = 0;
		bool _isTextEdited = false;
        std::string _placeholderText;
        std::string _text;

        void _onGeometryChange() override;

        void _onRender() override;

		void _updateRenderedText();
	    void _moveCursor(int p_delta);
        void _updateCursorBox();
        void _updateSelectionStatus();
        void _appendToText(const wchar_t& newChar);
        void _removeFromText();    
        void _deleteFromText();
        void _onUpdate() override;

    public:
        TextEntry(Widget* p_parent = nullptr);

        void setPlaceholder(const std::string& p_placeholderText);
        void setPredefinedText(const std::string& p_text);

        spk::WidgetComponent::TextLabel& label();
        spk::WidgetComponent::ColoredBox& cursor();
        spk::WidgetComponent::NineSlicedBox& box();

        const std::string& placeholder() const;
        const std::string& text() const;
    };
}