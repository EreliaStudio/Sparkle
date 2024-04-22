#pragma once

#include "widget/spk_widget.hpp"
#include "widget/components/spk_texture_renderer.hpp"

namespace spk
{
    /**
     * @class ImageLabel
     * @brief A widget that displays an image, using texture rendering.
     *
     * ImageLabel extends spk::Widget to provide functionality specifically for displaying images
     * within a user interface. It utilizes the spk::WidgetComponent::TextureRenderer to handle the
     * actual rendering of the image. This class is suitable for any UI elements that need to display
     * static images, icons, or other graphic content.
     *
     * The class supports dynamic resizing and positioning, making it versatile for various UI layouts.
     * It can be instantiated with a specific image during creation or have the image set or changed at runtime.
     *
     * Usage example:
     * @code
     * spk::ImageLabel imageLabel("logo.png", parentWidget);
     * imageLabel.label().setTexture("path/to/logo.png");
     * @endcode
     *
     * @note The texture renderer must be properly initialized with a valid texture before the ImageLabel is rendered.
     */
    class ImageLabel : public spk::Widget
    {
    private:
        WidgetComponent::TextureRenderer _renderer; // Handles the texture rendering of the image.

        void _onGeometryChange();
        void _onRender();

    public:
        /**
         * Constructs an ImageLabel widget with a parent.
         * @param p_parent The parent widget of this ImageLabel.
         */
        ImageLabel(spk::Widget* p_parent);

        /**
         * Constructs an ImageLabel widget with a name and a parent.
         * @param p_name The name of the widget, used for identification and debugging.
         * @param p_parent The parent widget of this ImageLabel.
         */
        ImageLabel(const std::string& p_name, spk::Widget* p_parent);

        /**
         * Provides non-const access to the internal texture renderer component.
         * @return Reference to the non-const TextureRenderer used by this ImageLabel.
         */
        WidgetComponent::TextureRenderer& label();

        /**
         * Provides const access to the internal texture renderer component.
         * @return Reference to the const TextureRenderer used by this ImageLabel.
         */
        const WidgetComponent::TextureRenderer& label() const;
    };
}