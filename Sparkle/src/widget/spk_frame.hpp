#pragma once

#include "widget/spk_widget.hpp"
#include "widget/components/spk_nine_sliced.hpp"

namespace spk
{
	/**
	 * @class Frame
	 * @brief A frame widget that contains a nine-sliced box for UI rendering.
	 *
	 * The Frame class extends the spk::Widget class and encapsulates a nine-sliced box component,
	 * enabling the creation of UI frames that are scalable without distorting their contents. It is
	 * particularly useful for UI elements like panels, windows, or dialog boxes that require
	 * flexible sizing while maintaining visual integrity.
	 *
	 * Key features include:
	 * - Scalable and adaptable frame geometry.
	 * - Integration with the SPK engine's widget system for consistent behavior across the UI.
	 * - Uses spk::WidgetComponent::NineSlicedBox for detailed texture slicing.
	 *
	 * Usage example:
	 * @code
	 * spk::Frame myFrame(parentWidget);
	 * myFrame.box().setSpriteSheet(mySpriteSheet);
	 * myFrame.box().setCornerSize(spk::Vector2Int(32, 32));
	 * myFrame.box().setGeometry(spk::Vector2Int(10, 10), myFrame.size());
	 * myFrame.activate();
	 * @endcode
	 *
	 * @note Ensure that the nine-sliced box is properly initialized and configured to match the frame's requirements.
	 */
	class Frame : public spk::Widget
	{
	private:
		spk::WidgetComponent::NineSlicedBox _box;

		void _onGeometryChange();
		void _onRender();

	public:
		/**
		 * Constructs a Frame widget with a given parent.
		 * @param p_parent The parent widget of this frame.
		 */
		Frame(spk::Widget* p_parent);

		/**
		 * Constructs a Frame widget with a specific name and a given parent.
		 * @param p_name The desired name of the widget.
		 * @param p_parent The parent widget of this frame.
		 */
		Frame(const std::string& p_name, spk::Widget* p_parent);

		/**
		 * Provides non-const access to the internal nine-sliced box component.
		 * @return Reference to the non-const NineSlicedBox used by the frame.
		 */
		spk::WidgetComponent::NineSlicedBox& box();

		/**
		 * Provides const access to the internal nine-sliced box component.
		 * @return Reference to the const NineSlicedBox used by the frame.
		 */
		const spk::WidgetComponent::NineSlicedBox& box() const;
	};
}
