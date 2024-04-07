#include "widget/spk_panel.hpp"

namespace spk
{
    Panel::Panel(spk::Widget* p_parent = nullptr) :
        spk::Widget(p_parent)
    {
        setActivationCallback([&](){
            if (_activePanel != nullptr)
                _activePanel->deactivate();
            _activePanel = this;
        });
    }
}