#pragma once

#include "widget/spk_pointer_detector.hpp"
#include "widget/spk_text_label.hpp"
#include "widget/spk_widget.hpp"

namespace spk::widget
{
    class TextButton : public IWidget
    {
    public:
        using Callback = std::function<void()>;
        TextButton(const std::string& p_text, const Callback& p_onPressed, spk::Font* p_font, IWidget* p_parent) :
            IWidget("TextButton", p_parent),
            _text(p_text),
            _font(p_font),
            _onPressed(p_onPressed)
        {
            _ptrDetector = std::make_unique<PointerDetector>(this);
            _ptrDetector->setOnPressed(_onPressed);

            _label = std::make_unique<TextLabel>("TextButton::TextLabel", _ptrDetector.get());
            _label->label().setFont(_font); // Assuming myFont is a preloaded Font instance
            _label->label().setText(_text);
            _label->label().setTextSize(24);
            _label->label().setTextColor(spk::Color{255, 255, 255});
            _label->label().setAnchor({10, 10});
            _label->label().setVerticalAlignment(spk::VerticalAlignment::Centered);
            _label->label().setHorizontalAlignment(spk::HorizontalAlignment::Left);
        }

    private:
        void _onGeometryChange() override
        {
        }

        std::unique_ptr<spk::widget::TextLabel> _label;
        std::unique_ptr<spk::widget::PointerDetector> _ptrDetector;

        std::string _text;
        spk::Font* _font;
        Callback _onPressed;
    };
}