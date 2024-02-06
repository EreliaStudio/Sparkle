#pragma once

#include "widget/spk_widget.hpp"
#include "game_engine/spk_game_engine.hpp"

namespace spk
{
	/**
	 * @brief Manages the integration of a game engine within a widget-based user interface framework.
	 *
	 * This class is a specialized widget that serves as a bridge between the game engine and the user interface,
	 * allowing for the game's visual content to be embedded within a UI widget hierarchy. It extends the IWidget class,
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
	class GameEngineManager : public spk::IWidget
	{
	private:
		spk::GameEngine* _engine = nullptr;

		void _onGeometryChange();
		void _onRender();
		void _onUpdate();

	public:
		GameEngineManager(const std::string& p_name);
		GameEngineManager(const std::string& p_name, spk::IWidget* p_parent);

		void setGameEngine(spk::GameEngine* p_engine);
	};
}