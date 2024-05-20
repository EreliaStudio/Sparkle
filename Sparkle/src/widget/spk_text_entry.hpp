#pragma once

#include "math/spk_vector2.hpp"
#include "spk_basic_functions.hpp"
#include "widget/components/spk_nine_sliced.hpp"
#include "widget/components/spk_text_renderer.hpp"
#include "widget/components/spk_colored_box.hpp"
#include "widget/spk_widget.hpp"
#include "input/spk_input.hpp"
#include <string>
#include <vector>

namespace spk
{
	/**
	 * @class TextEntry
	 * @brief A user interface component for text input.
	 *
	 * This class extends Widget to provide a text entry field that users can interact with.
	 * It features a nine-sliced box for the background, a text label for displaying the current text,
	 * and a colored box as the cursor. The TextEntry handles all aspects of text editing including
	 * cursor movement, text selection, and basic text manipulation (insert, delete).
	 *
	 * The widget can display a placeholder text when the actual text content is empty, which helps guide the user.
	 *
	 * Example usage:
	 * @code
	 * spk::TextEntry* myTextEntry = new spk::TextEntry(parentWidget);
	 * myTextEntry->setPlaceholder("Enter text here...");
	 * @endcode
	 *
	 * @note The TextEntry class manages keyboard inputs internally to update text based on user interactions.
	 */
	class TextEntry : public Widget
	{
	private:
		spk::WidgetComponent::NineSlicedBox _box; ///< Background box, resizable according to text length.
		spk::WidgetComponent::ColoredBox _cursorBox; ///< Cursor representation within the text box.
		spk::WidgetComponent::TextRenderer _label; ///< Label for displaying text content.

		std::vector<spk::Input> _inputs; ///< Collection of input handlers for managing keyboard input.

		bool _isSelected = false; ///< Flag to indicate if the text entry is currently selected.
		size_t _currentCursorPosition = 0; ///< Current position of the cursor within the text.
		size_t _openingCursorPosition = 0; ///< Start of the text selection range.
		size_t _closingCursorPosition = 0; ///< End of the text selection range.
		bool _isTextEdited = false; ///< Flag to check if the text has been edited.
		std::string _placeholderText; ///< Placeholder text displayed when the actual text is empty.
		std::string _text; ///< The actual text content of the widget.

		void _onGeometryChange() override; ///< Handles changes to the widget's geometry.
		void _onRender() override; ///< Render the text entry widget.

		void _updateRenderedText(); ///< Update text display based on current content and cursor position.
		void _moveCursor(int p_delta); ///< Move cursor position by a specified delta.
		void _updateCursorBox(); ///< Update the visual position and size of the cursor box.
		void _updateSelectionStatus(); ///< Update the selection status based on cursor movements.
		void _appendToText(const wchar_t& newChar); ///< Append a character to the text.
		void _removeFromText(); ///< Remove a character at the current cursor position.
		void _deleteFromText(); ///< Delete a character ahead of the current cursor position.
		void _onUpdate() override; ///< Update method to handle logic per frame.

	public:
		/**
		 * @brief Construct a new TextEntry widget with a given parent.
		 * @param p_parent Pointer to the parent widget.
		*/
		TextEntry(Widget* p_parent);

		/**
		 * @brief Construct a new TextEntry widget with a specific name and a given parent.
		 * @param p_name The desired name of the widget.
		 * @param p_parent Pointer to the parent widget.
		*/
		TextEntry(const std::string& p_name, Widget* p_parent);

		/**
		 * Sets the placeholder text to be displayed when there is no user input.
		 * @param p_placeholderText The placeholder text.
		 */
		void setPlaceholder(const std::string& p_placeholderText);

		/**
		 * Sets predefined text in the text entry widget.
		 * @param p_text The text to set initially.
		 */
		void setPredefinedText(const std::string& p_text);

		/**
		 * Access the text label component of the text entry.
		 * @return Reference to the internal TextLabel component.
		 */
		spk::WidgetComponent::TextRenderer& label();

		/**
		 * Access the cursor box component of the text entry.
		 * @return Reference to the internal ColoredBox component used as the cursor.
		 */
		spk::WidgetComponent::ColoredBox& cursor();

		/**
		 * Access the nine-sliced box component of the text entry.
		 * @return Reference to the internal NineSlicedBox component.
		 */
		spk::WidgetComponent::NineSlicedBox& box();

		/**
		 * Retrieves the current placeholder text.
		 * @return The placeholder text as a const std::string reference.
		 */
		const std::string& placeholder() const;

		/**
		 * Retrieves the current text content of the text entry.
		 * @return The text content as a const std::string reference.
		 */
		const std::string& text() const;
	};
}