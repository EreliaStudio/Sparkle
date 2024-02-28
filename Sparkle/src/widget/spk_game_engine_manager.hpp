#pragma once

#include "game_engine/spk_game_engine.hpp"
#include "widget/spk_widget.hpp"

namespace spk::widget
{
    /**
     * @class GameEngineManager
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
    class GameEngineManager : public IWidget
    {
    private:
        spk::GameEngine* _engine = nullptr;

        void _onGeometryChange() override;
        void _onRender() override;
        void _onUpdate() override;

    public:
        /**
         * @brief Constructs a GameEngineManager widget with a specified name.
         *
         * Initializes a new instance of GameEngineManager without attaching it to a parent widget. This constructor
         * sets the name of the widget but does not immediately integrate it into the UI hierarchy. The GameEngine instance
         * must be set separately using setGameEngine.
         *
         * @param p_name The name of the GameEngineManager widget, used for identification within the UI hierarchy.
         */
        GameEngineManager(const std::string& p_name);

        /**
         * @brief Constructs a GameEngineManager widget with a specified name and parent widget.
         *
         * Initializes a new instance of GameEngineManager and attaches it to a parent widget, effectively integrating
         * it into the UI hierarchy. This constructor allows for immediate placement of the GameEngineManager within
         * the widget tree, facilitating its rendering and event handling within the context of the parent widget.
         *
         * @param p_name The name of the GameEngineManager widget, used for identification within the UI hierarchy.
         * @param p_parent A pointer to the parent IWidget, determining the hierarchical placement of this GameEngineManager.
         */
        GameEngineManager(const std::string& p_name, IWidget* p_parent);

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
    using GameEngineManager = spk::widget::GameEngineManager;
}