#pragma once

#include <vector>
#include <stdexcept>
#include "math/spk_vector2.hpp"
#include "spk_basic_functions.hpp"
#include "widget/components/spk_nine_sliced.hpp"
#include "widget/components/spk_sprite_sheet_renderer.hpp"
#include "widget/spk_widget.hpp"

namespace spk
{
	/**
	 * @class StatefulWidget
	 * @brief A widget class that links states to specific sprites and renders a background.
	 *
	 * This class extends Widget and includes a SpriteSheetRenderer for rendering sprites and a
	 * NineSlicedBox for the background. It allows the user to define states and link each state
	 * to a specific sprite ID within a sprite sheet.
	 *
	 * The class manages the graphical representation and interaction logic for stateful elements
	 * in the UI, supporting dynamic resizing and state changes.
	 *
	 * Usage example:
	 * @code
	 * spk::StatefulWidget<std::string> statefulWidget(parentWidget);
	 * statefulWidget.addStateSprite("active", 1);
	 * statefulWidget.addStateSprite("inactive", 2);
	 * statefulWidget.nextState(); // Cycles to the next state
	 * @endcode
	 */
	template <typename State>
	class StatefulWidget : public Widget
	{
	private:
		spk::WidgetComponent::NineSlicedBox _box; ///< Background box that can be styled and resized.
		spk::WidgetComponent::SpriteSheetRenderer _spriteRenderer; ///< Sprite renderer component.

		std::vector<std::pair<State, int>> _stateSpritePairs; ///< Vector linking states to sprite IDs.
		size_t _currentIndex; ///< Current index of the state in the vector.

		/**
		 * Handles geometry changes, updating the layout of the sprite and background to fit the new widget size.
		 */
		void _onGeometryChange()
		{
			_box.setGeometry(anchor(), size());
			_box.setLayer(layer());

			_spriteRenderer.setGeometry(anchor() + _box.cornerSize(), size() - _box.cornerSize() * 2);
			_spriteRenderer.setLayer(layer() + 0.01f);
		}

		/**
		 * Renders the sprite and its background to the screen.
		 */
		void _onRender()
		{
			_box.render();
			_spriteRenderer.render();
		}

		/**
		 * Updates the widget state based on interactions.
		 */
		void _onUpdate()
		{
			const auto& mouse = Application::activeApplication()->mouse();

			if (mouse.getButton(Mouse::Left) == InputState::Released && hitTest(mouse.position()))
			{
				nextState();
			}
		}

	public:
		/**
		 * @brief Construct a new StatefulWidget widget with a given parent.
		 * @param p_parent Pointer to the parent widget.
		 */
		StatefulWidget(Widget* p_parent) :
			StatefulWidget("Anonymous StatefulWidget", p_parent)
		{
			_currentIndex = 0;
		}
		
		/**
		 * @brief Construct a new StatefulWidget widget with a specific name and a given parent.
		 * @param p_name The desired name of the widget.
		 * @param p_parent Pointer to the parent widget.
		 */
		StatefulWidget(const std::string& p_name, Widget* p_parent) :
			spk::Widget(p_name, p_parent),
			_currentIndex(0)
		{

		}

		/**
		 * Adds a state and its corresponding sprite ID to the widget.
		 * @param state The state to link to a sprite.
		 * @param spriteID The sprite ID to be displayed for the given state.
		 */
		void addStateSprite(const State& state, int spriteID)
		{
			_stateSpritePairs.push_back(std::make_pair(state, spriteID));
			if (_stateSpritePairs.size() == 1)
			{
				_spriteRenderer.setSprite(spriteID); // Set initial sprite if this is the first state added.
			}
		}

		/**
		 * Cycles to the next state in the vector.
		 */
		void nextState()
		{
			if (_stateSpritePairs.empty())
			{
				throw std::out_of_range("No states have been added to the StatefulWidget.");
			}

			_currentIndex = (_currentIndex + 1) % _stateSpritePairs.size();
			_spriteRenderer.setSprite(_stateSpritePairs[_currentIndex].second);
		}

		/**
		 * Provides non-const access to the internal SpriteSheetRenderer component.
		 * @return Reference to the non-const SpriteSheetRenderer used by this widget for displaying sprites.
		 */
		spk::WidgetComponent::SpriteSheetRenderer& spriteRenderer()
		{
			return (_spriteRenderer);
		}

		/**
		 * Provides non-const access to the internal NineSlicedBox component.
		 * @return Reference to the non-const NineSlicedBox used by this widget for the background.
		 */
		spk::WidgetComponent::NineSlicedBox& box()
		{
			return (_box);
		}
	};
}