#pragma once

#include "widget/spk_widget.hpp"
#include <functional>

namespace spk
{
    /**
     * @class Panel
     * @brief A user interface panel that can manage focus within the application.
     *
     * Panel extends spk::Widget to provide functionality for managing an active panel within the UI.
     * It maintains a static pointer to the currently active panel and supports a callback mechanism
     * that triggers when the panel becomes active. This class is designed to handle focus transitions
     * between different panel components in a user interface, allowing only one panel to be active at a time.
     *
     * The activation logic ensures that any previously active panel is deactivated before this one
     * becomes active, maintaining a clean and controlled focus state within the application.
     *
     * Usage example:
     * @code
     * spk::Panel* settingsPanel = new spk::Panel(parentWidget);
     * settingsPanel->setPanelActivationCallback([]() {
     *     std::cout << "Settings panel is now active." << std::endl;
     * });
     * @endcode
     *
     * @note Setting the panel activation callback allows custom actions to be performed when the panel
     * becomes the active focus within the application.
     */
    class Panel : public spk::Widget
    {
    private:
        static inline Panel* _activePanel = nullptr; ///< Pointer to the currently active panel.

        std::function<void()> _onPanelActivationCallback; ///< Callback function executed when this panel is activated.

    public:
        /**
         * Constructs a Panel with an optional parent widget.
         * @param p_parent Pointer to the parent widget, which can be nullptr if the panel has no parent.
         */
        Panel(spk::Widget* p_parent);
		
        Panel(const std::string& p_name, spk::Widget* p_parent);

        /**
         * Sets a callback function that is executed when this panel becomes active.
         * @param p_callback A std::function<void()> that specifies the action to perform upon activation.
         */
        void setPanelActivationCallback(const std::function<void()>& p_callback);
    };
}
