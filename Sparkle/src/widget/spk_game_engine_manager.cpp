#include "widget/spk_game_engine_manager.hpp"
#include "game_engine/component/spk_camera_component.hpp"

namespace spk::widget
{
    void GameEngineManager::_onGeometryChange()
    {
        if (spk::Camera::mainCamera() != nullptr)
            spk::Camera::mainCamera()->setAspectRatio(static_cast<float>(size().x) / static_cast<float>(size().y));
    }

    void GameEngineManager::_onRender()
    {
        if (_engine != nullptr)
        {
            _engine->render();
        }
    }

    void GameEngineManager::_onUpdate()
    {
        if (_engine != nullptr)
        {
            _engine->update();
        }
    }

    GameEngineManager::GameEngineManager(const std::string& p_name) :
        IWidget(p_name)
    {
    }

    GameEngineManager::GameEngineManager(const std::string& p_name, IWidget* p_parent) :
        IWidget(p_name, p_parent)
    {
    }

    void GameEngineManager::setGameEngine(spk::GameEngine* p_engine)
    {
        _engine = p_engine;
    }
}