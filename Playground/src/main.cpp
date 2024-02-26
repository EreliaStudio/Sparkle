#include "sparkle.hpp"
#include <iostream>
#include <limits>

class MainWidget : public spk::IWidget
{
private:
    spk::SpriteSheet _backgroundTilemapTexture;
    spk::SpriteSheet _playerSpriteSheet;
    spk::GameEngineManager _gameEngineManager;

    spk::GameEngine _gameEngine;

    spk::GameObject _playerObject;
    spk::MeshRenderer *_playerBodyRenderer;

    spk::GameObject _cameraObject;
    spk::Camera* _cameraComponent;

    spk::GameObject _backgroundTilemap;
    spk::Tilemap2D* _tilemapComponent;

    spk::Vector3 _cameraOffset = 0;

    void _onGeometryChange()
    {
        _gameEngineManager.setGeometry(anchor(), size());
        _cameraComponent->setOrthographicSize(size() / 64);
    }

    void _onUpdate()
    {
        if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::Z) == spk::InputState::Pressed)
        {
            _playerObject.transform().translation += spk::Vector3(0, 0.5f, 0);
        }
        if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::S) == spk::InputState::Pressed)
        {
            _playerObject.transform().translation += spk::Vector3(0, -0.5f, 0);
        }
        if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::Q) == spk::InputState::Pressed)
        {
            _playerObject.transform().translation += spk::Vector3(-0.5f, 0, 0);
        }
        if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::D) == spk::InputState::Pressed)
        {
            _playerObject.transform().translation += spk::Vector3(0.5f, 0, 0);
        }

        if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::Space) == spk::InputState::Pressed)
        {
            std::cout << "Position : " << _playerObject.transform().translation.get() << std::endl;
        }

        if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::A) == spk::InputState::Pressed)
        {
            _cameraObject.transform().rotation *= spk::Quaternion::fromEulerAngles(0, -10, 0);
        }
        if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::E) == spk::InputState::Pressed)
        {
            _cameraObject.transform().rotation *= spk::Quaternion::fromEulerAngles(0, 10, 0);
        }

        if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::W) == spk::InputState::Pressed)
        {
            _cameraObject.transform().rotation *= spk::Quaternion::fromEulerAngles(-10, 0, 0);
        }
        if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::X) == spk::InputState::Pressed)
        {
            _cameraObject.transform().rotation *= spk::Quaternion::fromEulerAngles(10, 0, 0);
        }

        if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::C) == spk::InputState::Pressed)
        {
            _cameraObject.transform().rotation *= spk::Quaternion::fromEulerAngles(0, 0, -10);
        }
        if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::V) == spk::InputState::Pressed)
        {
            _cameraObject.transform().rotation *= spk::Quaternion::fromEulerAngles(0, 0, 10);
        }
    }

public:
    MainWidget(const std::string& p_name) :
        spk::IWidget(p_name),
        _backgroundTilemapTexture("Playground/worldBackground.png", spk::Vector2Int(12, 6)),
        _playerSpriteSheet("Playground/playerSpriteSheet.png", spk::Vector2Int(4, 4)),
        _gameEngineManager("GameEngineManager", this),
        _playerObject("Player"),
        _playerBodyRenderer(_playerObject.addComponent<spk::MeshRenderer>("BodyRenderer")),
        _cameraObject("Camera", &_playerObject),
        _cameraComponent(_cameraObject.addComponent<spk::Camera>("Camera")), 
        _backgroundTilemap("BackgroundTilemap"),
        _tilemapComponent(_backgroundTilemap.addComponent<spk::Tilemap2D>("Tilemap"))
    {
        _playerObject.transform().translation = spk::Vector3(0, 0, 2.5f);
        _playerBodyRenderer->setSpriteSheet(&_playerSpriteSheet);
        _playerBodyRenderer->setSprite(spk::Vector2Int(0, 0));

        _cameraObject.transform().translation = spk::Vector3(0, 0, 5);  
        _cameraObject.transform().lookAt(_playerObject.transform().translation.get());
        _cameraComponent->setAsMainCamera();
        _cameraComponent->setType(spk::Camera::Type::Orthographic);
        _cameraComponent->setNearPlane(0);
        _cameraComponent->setFarPlane(8);
        _cameraComponent->setOrthographicSize(10);
        _cameraComponent->setAspectRatio(1.0f);
        _cameraComponent->setFOV(75);

        _backgroundTilemap.transform().translation = spk::Vector3(0, 0, 0);

        _tilemapComponent->setSpriteSheet(&_backgroundTilemapTexture);
        _tilemapComponent->createEmpyChunk(spk::Vector2Int(0, 0));
        // _tilemapComponent->createEmpyChunk(spk::Vector2Int(1, 0));
        // _tilemapComponent->createEmpyChunk(spk::Vector2Int(0, 1));
        // _tilemapComponent->createEmpyChunk(spk::Vector2Int(1, 1));
        _tilemapComponent->insertNodeType(0, spk::Tilemap2D::Node(spk::Vector2Int(0, 0), spk::Tilemap2D::Node::OBSTACLE, true));
        _tilemapComponent->insertNodeType(1, spk::Tilemap2D::Node(spk::Vector2Int(4, 0), spk::Tilemap2D::Node::WALKABLE, false));
        _tilemapComponent->updateActiveChunks();
        
        _gameEngine.subscribe(&_playerObject);
        _gameEngine.subscribe(&_backgroundTilemap);
        _gameEngineManager.setGameEngine(&_gameEngine);
        _gameEngineManager.activate();  
    }

    ~MainWidget()
    {

    }
};

int main()
{
    spk::Application app = spk::Application("Labi", spk::Vector2UInt(800, 800), spk::Application::Mode::Multithread);

    MainWidget mainWidget = MainWidget("MainWidget");
    mainWidget.setGeometry(0, app.size());
    mainWidget.activate();

    return (app.run());
}
