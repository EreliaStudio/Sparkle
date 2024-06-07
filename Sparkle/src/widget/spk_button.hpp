#pragma once

#include "widget/spk_widget.hpp"
#include "design_pattern/spk_state_machine.hpp"

#include "widget/components/spk_nine_sliced.hpp"
#include "widget/components/spk_font_renderer.hpp"

namespace spk
{
	/**
	 * @class Button
	 * @brief Represents a button widget in the SPK UI framework, capable of handling user interactions and displaying different states.
	 *
	 * This class models a button that can handle basic user interactions within the SPK UI framework.
	 * It leverages the StateMachine class to manage button states such as pressed and released. Each state has
	 * a corresponding visual representation through NineSlicedBox and TextLabel components, which adjust based
	 * on the button's current state. The Button class provides methods to set callback actions on click events,
	 * manipulate text labels, and directly access the visual components for each state.
	 *
	 * Key features include:
	 * - State management through an embedded state machine.
	 * - Customizable text and background visuals for different states.
	 * - Callback mechanism to handle click events.
	 *
	 * Usage example:
	 * @code
	 * spk::Button myButton(parentWidget);
	 * myButton.setOnClickCallback([]() { std::cout << "Button clicked!" << std::endl; });
	 * myButton.setText("Click Me!");
	 * myButton.render();
	 * @endcode
	 *
	 * @note Ensure that callback functions do not capture overly large scopes or perform intensive computations
	 * to avoid performance degradation.
	 *
	 * @see StateMachine, NineSlicedBox, TextLabel
	 */
	class Button : public Widget
	{
	public:
		/**
		 * @brief Defines a type for the callback function used for click events.
		 * 
		 * The Callback type is used to store a function or a callable object that takes no parameters and returns void.
		 */
		using Callback = std::function<void()>;

		/**
		 * @enum State
		 * @brief Defines the possible states of the Button, such as pressed and released.
		 */
		enum class State
		{
			Pressed = 0,
			Released = 1
		};

	private:
		StateMachine<State> _stateMachine;

		WidgetComponent::NineSlicedBox _boxes[2];
		WidgetComponent::FontRenderer _labels[2];
		spk::Vector2Int _padding;

		Callback _onClickCallback;

		void _onGeometryChange() override;
		void _onRender() override;
		void _onUpdate() override;

	public:
		/**
		 * @brief Constructs a new Button widget with the given parent.
		 * @param p_parent Pointer to the parent widget.
		 */
		Button(Widget* p_parent);

		/**
		 * @brief Construct a nes button widget with a specific name and a given parent.
		 * @param p_name The desired name of the widget.
		 * @param p_parent Pointer to the parent widget.
		*/
		Button(const std::string& p_name, Widget* p_parent);

		spk::Font::Size computeOptimalFontSize(const float& p_ratio);

		void setPadding(const spk::Vector2Int& p_padding);

		/**
		 * @brief Sets the callback function to be called when the button is clicked.
		 * @param p_onClickCallback The function to call on a button click.
		 */
		void setOnClickCallback(const Callback p_onClickCallback);

		/**
		 * @brief Retrieves the visual box component associated with a specified state.
		 * @param p_state The state for which to retrieve the box.
		 * @return A reference to the NineSlicedBox component for the specified state.
		 */
		WidgetComponent::NineSlicedBox& box(const State& p_state);

		/**
		 * @brief Retrieves the text label component associated with a specified state.
		 * @param p_state The state for which to retrieve the label.
		 * @return A reference to the TextLabel component for the specified state.
		 */
		WidgetComponent::FontRenderer& label(const State& p_state);

		/**
		 * @brief Sets the text of the button labels for all states.
		 * @param p_buttonText The text to set on the button.
		 */
		void setText(const std::string& p_buttonText);
	}; 
}