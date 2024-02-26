#include "sparkle.hpp"

enum class Event
{
    UpdateVisibleChunk
};

class TextureManager : public spk::Singleton<TextureManager>
{
    friend class spk::Singleton<TextureManager>;

private:
    std::map<std::string, std::unique_ptr<spk::Texture>> _textures;

    TextureManager()
    {

    }

public:
    template<typename TTextureType, typename ... Args>
    void add(const std::string& p_name, Args&& ... p_constructorArguments)
    {
        static_assert(std::is_base_of<spk::Texture, TTextureType>::value, "TTextureType must be a derivative of spk::Texture");

        if (_textures.contains(p_name))
        {
            _textures[p_name] = std::make_unique<TTextureType>(std::forward<Args>(p_constructorArguments)...);
        }
        else
        {
            _textures.insert(std::make_pair(p_name, std::make_unique<TTextureType>(std::forward<Args>(p_constructorArguments)...)));
        }
    }

    const spk::Texture* texture(const std::string& p_name) const
    {
        if (_textures.contains(p_name) == false)
            return (nullptr);
        return (_textures.at(p_name).get());        
    }
    
    template<typename TTextureType>
    const TTextureType* as(const std::string& p_name)
    {
        static_assert(std::is_base_of<spk::Texture, TTextureType>::value, "TTextureType must be a derivative of spk::Texture");

        return (dynamic_cast<const TTextureType*>(texture(p_name)));
    }
};

using EventController = spk::Singleton<spk::EventManager<Event>>;

class PlayerController : public spk::IWidget
{
private:
    spk::GameObject _playerObject;
    spk::GameObject _cameraObject;
    spk::Camera* _cameraComponent;
    spk::MeshRenderer *_playerBodyRenderer;

    void _onGeometryChange()
    {
        _cameraComponent->setOrthographicSize(size() / 64);
        EventController::instance()->notify_all(Event::UpdateVisibleChunk);
    }

    void _onUpdate()
    {
        static long long nextInput = spk::getTime();
        long long currentNextInput = nextInput;
        long long time = spk::getTime();

        spk::Keyboard::Key keys[4] = {
            spk::Keyboard::Z,
            spk::Keyboard::S,
            spk::Keyboard::Q,
            spk::Keyboard::D
        };

        spk::Vector3 deltas[4] = {
            spk::Vector3(0, 1, 0),
            spk::Vector3(0, -1, 0),
            spk::Vector3(-1, 0, 0),
            spk::Vector3(1, 0, 0)
        };

        spk::Vector3 nextDelta = 0;

        for (size_t i = 0; i < 4; i++)
        {
            if (spk::Application::activeApplication()->keyboard().getKey(keys[i]) == spk::InputState::Down && 
                currentNextInput <= spk::getTime())
            {
                nextDelta += deltas[i];
                nextInput = time + 50;
            }
        }

        if (nextDelta != 0)
        {
            _playerObject.transform().translation += nextDelta;
            EventController::instance()->notify_all(Event::UpdateVisibleChunk);
        }
    }

public:
    PlayerController(const std::string& p_name, spk::IWidget* p_parent) :
        spk::IWidget(p_name, p_parent),
        _playerObject("Player"),
        _cameraObject("Camera", &_playerObject),
        _cameraComponent(_cameraObject.addComponent<spk::Camera>("Camera")),
        _playerBodyRenderer(_playerObject.addComponent<spk::MeshRenderer>("BodyRenderer"))
    {
        _playerObject.transform().translation = spk::Vector3(0, 0, 2.5f);
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
    }

    spk::GameObject& playerObject()
    {
        return (_playerObject);
    }

    spk::Camera* cameraComponent()
    {
        return (_cameraComponent);
    }

    spk::MeshRenderer* playerBodyRenderer()
    {
        return (_playerBodyRenderer);
    }
};

class TilemapManager : public spk::IWidget
{
private:
    spk::GameObject _tilemapObject;
    spk::Tilemap2D* _tilemapComponent;
    std::unique_ptr<EventController::Type::Contract> _onUpdateVisibleChunkContract;
    bool _needActiveChunkUpdate = true;

    void _updateChunkVisibleOnScreen()
    {
        if (spk::Camera::mainCamera() == nullptr)
            return ;

        spk::Vector2 nbTileOnScreen = spk::Vector2::ceiling(spk::Camera::mainCamera()->orthographicSize() / 2.0f);

        spk::Vector2Int playerPosition = spk::Camera::mainCamera()->owner()->globalPosition().xy();

        spk::Vector2 start = spk::Tilemap2D::convertWorldToChunkPosition(playerPosition - nbTileOnScreen);
        spk::Vector2 end = spk::Tilemap2D::convertWorldToChunkPosition(playerPosition + nbTileOnScreen);

        _tilemapComponent->setActiveChunkRange(start, end);

        static spk::Vector2Int chunkOffsets[8] = {
            spk::Vector2Int(-1, -1),
            spk::Vector2Int(0, -1),
            spk::Vector2Int(1, -1),

            spk::Vector2Int(-1, 0),
            spk::Vector2Int(1, 0),

            spk::Vector2Int(-1, 1),
            spk::Vector2Int(0, 1),
            spk::Vector2Int(1, 1)
        };

        for (const auto& element : _tilemapComponent->missingChunks())
        {
            spk::Tilemap2D::Chunk* newChunk = dynamic_cast<spk::Tilemap2D::Chunk*>(_tilemapComponent->createEmpyChunk(element));

            for (size_t i = 0; i < 8; i++)
            {
                spk::GameObject* chunkObject = _tilemapComponent->chunkObject(element + chunkOffsets[i]);
                if (chunkObject != nullptr)
                    chunkObject->getComponent<spk::Tilemap2D::Chunk>()->bake();
            }
        }
        _tilemapComponent->updateActiveChunks();
    }

    void _onRender()
    {
        if (_needActiveChunkUpdate == true)
        {
            _updateChunkVisibleOnScreen();
            _needActiveChunkUpdate = false;
        }
    }

public:
    TilemapManager(const std::string& p_name, spk::IWidget* p_parent) :
        spk::IWidget(p_name, p_parent),
        _tilemapObject("TilemapObject"),
        _tilemapComponent(_tilemapObject.addComponent<spk::Tilemap2D>("Tilemap")),
        _onUpdateVisibleChunkContract(EventController::instance()->subscribe(Event::UpdateVisibleChunk, [&](){
            _needActiveChunkUpdate = true;
        }))
    {
        _tilemapObject.transform().translation = spk::Vector3(0, 0, 0);
        
        _tilemapComponent->insertNodeType(0, spk::Tilemap2D::Node(spk::Vector2Int(0, 0), spk::Tilemap2D::Node::OBSTACLE, true));
        _tilemapComponent->insertNodeType(1, spk::Tilemap2D::Node(spk::Vector2Int(4, 0), spk::Tilemap2D::Node::WALKABLE, false));
    }

    spk::GameObject& tilemapObject()
    {
        return (_tilemapObject);
    }

    spk::Tilemap2D* tilemapComponent()
    {
        return (_tilemapComponent);
    }
};

class MainWidget : public spk::IWidget
{
private:
    spk::GameEngine _gameEngine;

    spk::GameEngineManager _gameEngineManager;
    PlayerController _playerController;
    TilemapManager _tilemapManager;

    void _onGeometryChange()
    {
        _gameEngineManager.setGeometry(anchor(), size());
        _playerController.setGeometry(anchor(), size());
        _tilemapManager.setGeometry(anchor(), size());
    }

public:
    MainWidget(const std::string& p_name) :
        spk::IWidget(p_name),
        _gameEngineManager("GameEngineManager", this),
        _playerController("PlayerController", this),
        _tilemapManager("TilemapManager", this)
    {
        _tilemapManager.tilemapComponent()->setSpriteSheet(TextureManager::instance()->as<spk::SpriteSheet>("WorldTileset"));

        _playerController.playerBodyRenderer()->setSpriteSheet(TextureManager::instance()->as<spk::SpriteSheet>("PlayerSprite"));

        _gameEngine.subscribe(&(_playerController.playerObject()));
        _gameEngine.subscribe(&(_tilemapManager.tilemapObject()));

        _gameEngineManager.setGameEngine(&_gameEngine);
        _gameEngineManager.activate(); 
        _playerController.activate(); 
        _tilemapManager.activate();
    }

    ~MainWidget()
    {

    }
};

int executeGame()
{
    spk::Application app = spk::Application("Labi", spk::Vector2UInt(800, 800), spk::Application::Mode::Multithread);

    TextureManager::instance()->add<spk::SpriteSheet>("PlayerSprite", "Playground/playerSpriteSheet.png", spk::Vector2Int(4, 4));
    TextureManager::instance()->add<spk::SpriteSheet>("WorldTileset", "Playground/worldBackground.png", spk::Vector2Int(16, 24));

    MainWidget mainWidget = MainWidget("MainWidget");
    mainWidget.setGeometry(0, app.size());
    mainWidget.activate();

    return (app.run());
}

int main()
{
    EventController::instanciate();
    TextureManager::instanciate();

    int errorCode = executeGame();

    TextureManager::release();
    EventController::release();

    return (errorCode);
}