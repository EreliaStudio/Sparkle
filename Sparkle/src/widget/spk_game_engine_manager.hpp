#pragma once

#include "game_engine/spk_game_engine.hpp"
#include "widget/spk_widget.hpp"

namespace spk
{
	/**
	 * @class GameEngineManager
	 * @brief Manages the integration of a game engine within a widget-based user interface framework.
	 *
	 * This class is a specialized widget that serves as a bridge between the game engine and the user interface,
	 * allowing for the game's visual content to be embedded within a UI widget hierarchy. It extends the Widget class,
	 * inheriting its hierarchical organization and viewport management capabilities, to seamlessly integrate game rendering
	 * and update cycles within the UI framework.
	 *
	 * This class encapsulates a pointer to a GameEngine instance, providing methods to set and manage the game engine's
	 * lifecycle in conjunction with the UI's rendering and update loops. By overriding the _onGeometryChange, _onRender,
	 * and _onUpdate methods, GameEngineManager ensures that the game engine's output is correctly rendered within its
	 * designated widget area and that game state updates occur in sync with the UI's update cycle.
	 *
	 * Usage example:
	 * @code
	 * spk::GameEngine myGameEngine;
	 * spk::GameEngineManager myGameEngineManager("GameView", parentWidget);
	 * myGameEngineManager.setGameEngine(&myGameEngine);
	 * myGameEngineManager.activate();
	 * // The game engine is now integrated into the UI hierarchy and will render within the GameEngineManager's viewport.
	 * @endcode
	 *
	 * @note GameEngineManager provides a flexible way to embed game content within application interfaces, enabling the
	 * development of games with complex UI overlays or applications with interactive game-based elements.
	 */
	class GameEngineManager : public Widget
	{
	private:
		spk::GameEngine* _engine = nullptr;

		void _onGeometryChange() override;
		void _onRender() override;
		void _onUpdate() override;

	public:

		/**
		 * @brief Construct a new GameEngineManager widget with a given parent.
		 * @param p_parent Pointer to the parent widget.
		*/
		GameEngineManager(Widget* p_parent);

		/**
		 * @brief Construct a new GameEngineManager widget with a specific name and a given parent.
		 * @param p_name The desired name of the widget.
		 * @param p_parent Pointer to the parent widget.
		*/
		GameEngineManager(const std::string& p_name, Widget* p_parent);

		/**
		 * @brief Sets the game engine to be managed and integrated within the UI framework.
		 *
		 * This method assigns a GameEngine instance to the GameEngineManager, enabling the manager to control the
		 * game engine's lifecycle, rendering, and updates in alignment with the UI's rendering and update cycles.
		 * The game engine's visual output will be rendered within the viewport of this GameEngineManager widget.
		 *
		 * @param p_engine A pointer to a GameEngine instance that should be managed by this GameEngineManager.
		 */
		void setGameEngine(spk::GameEngine* p_engine);
	};
}

namespace spk
{
	using GameEngineManager = spk::GameEngineManager;
}