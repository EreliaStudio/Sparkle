#include "widget/spk_panel.hpp"

namespace spk
{
    Panel::Panel(spk::Widget* p_parent) :
        spk::Widget(p_parent),
        _onPanelActivationCallback(nullptr)
    {
        setActivationCallback([&](){
            if (_activePanel != nullptr)
            {
                _activePanel->deactivate();
            }
            _activePanel = this;
            if (_onPanelActivationCallback != nullptr)
                _onPanelActivationCallback();
        });
    }
	
    void Panel::setPanelActivationCallback(const Callback& p_callback)
    {
        _onPanelActivationCallback = p_callback;
    }
}