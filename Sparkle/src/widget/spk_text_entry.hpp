#pragma once

#include "math/spk_vector2.hpp"
#include "spk_basic_functions.hpp"
#include "widget/components/spk_nine_sliced.hpp"
#include "widget/components/spk_text_label.hpp"
#include "widget/spk_widget.hpp"

namespace spk
{
	class TextEntry : public Widget
    {
    private:
        spk::WidgetComponent::NineSlicedBox _box;
        spk::WidgetComponent::TextLabel _label;

		bool _isSelected = false;
        size_t _cursorPosition = 0;

        void _onGeometryChange() override;

        void _onRender() override;

        void _updateSelectionStatus();
        void _appendToText(const wchar_t& newChar);
        void _removeFromText();    
        void _deleteFromText();    
        void _onUpdate() override;

    public:
        TextEntry(Widget* p_parent = nullptr);

        spk::WidgetComponent::TextLabel& label();

        spk::WidgetComponent::NineSlicedBox& box();
    };
}