#include "sparkle.hpp"

struct Node
{

};

class Tilemap2D : public spk::ITilemap<Node, short, 16, 16, 5>
{
public:
    class Chunk : public Tilemap2D::IChunk
    {
    private:
        spk::Vector2Int _position;

        void _bake(std::vector<VertexData>& p_vertexData, std::vector<unsigned int>& p_indexes) override
        {
            p_vertexData = {
                VertexData{
                    spk::Vector3(0.5f, 0.5f, 0.0f),
                    spk::Vector2(1, 0),
                    spk::Vector2(0, 0),
                    0
                },
                VertexData{
                    spk::Vector3(-0.5f, 0.5f, 0.0f),
                    spk::Vector2(0, 0),
                    spk::Vector2(0, 0),
                    0
                },
                VertexData{
                    spk::Vector3(0.5f, -0.5f, 0.0f),
                    spk::Vector2(1, 1),
                    spk::Vector2(0, 0),
                    0
                },
                VertexData{
                    spk::Vector3(-0.5f, -0.5f, 0.0f),
                    spk::Vector2(0, 1),
                    spk::Vector2(0, 0),
                    0
                }
            };

            p_indexes = {
                0, 1, 2, 2, 1, 3
            };
        }

    public:
        Chunk(const spk::Vector2Int& p_chunkPosition) :
            ITilemap::IChunk("Chunk [" + p_chunkPosition.to_string() + "]"),
            _position(p_chunkPosition)
        {
            owner()->transform().translation = spk::Vector3(static_cast<float>(p_chunkPosition.x * Chunk::SizeX), static_cast<float>(p_chunkPosition.y * Chunk::SizeY), 0.0f);
        }
    };

private:
    Chunk* _insertChunk(spk::GameObject* p_object, const spk::Vector2Int& p_chunkPosition) override
    {
        return (p_object->addComponent<Chunk>(p_chunkPosition));
    }

    std::vector<spk::GameObject*> _activeChunks;

    void _onPlayerChunkPositionModification()
    {
        std::vector<spk::GameObject*> chunksToActivate;

        spk::Vector2Int start = convertWorldToChunkPositionXY(spk::Camera::mainCamera()->owner()->globalPosition()) - spk::Vector2Int(1, 1);
        spk::Vector2Int end = start + spk::Vector2Int(2, 2);

        for (int x = start.x; x <= end.x; x++)
        {
            for (int y = start.y; y <= end.y; y++)
            {
                spk::GameObject* chunkToActivate = chunkObject(spk::Vector2Int(x, y));

                if (chunkToActivate != nullptr)
                    chunksToActivate.push_back(chunkToActivate);
            }
        }

        for (auto& element : _activeChunks)
        {
            if (std::find(chunksToActivate.begin(), chunksToActivate.end(), element) == chunksToActivate.end())
                element->deactivate();
        }

        _activeChunks = chunksToActivate;
        for (auto& element : _activeChunks)
        {
            if (element->isActive() == false)
                element->activate();
        }
    }

    void _onUpdate()
    {

    }

    void _onRender()
    {
        
    }

public:
    Tilemap2D(const std::string& p_name) :
        ITilemap(p_name)
    {

    }

    void updateActiveChunks()
    {
        _onPlayerChunkPositionModification();
    }
};

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
    Tilemap2D* _tilemapComponent;

    void _onGeometryChange()
    {
        _gameEngineManager.setGeometry(anchor(), size());
        _cameraComponent->setOrthographicSize(size() / 64);
    }

    void _onUpdate()
    {
        if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::Z) == spk::InputState::Pressed)
        {
            _playerObject.transform().translation += spk::Vector3(0, 1, 0);
        }
        if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::S) == spk::InputState::Pressed)
        {
            _playerObject.transform().translation += spk::Vector3(0, -1, 0);
        }
        if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::Q) == spk::InputState::Pressed)
        {
            _playerObject.transform().translation += spk::Vector3(-1, 0, 0);
        }
        if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::D) == spk::InputState::Pressed)
        {
            _playerObject.transform().translation += spk::Vector3(1, 0, 0);
        }
    }

public:
    MainWidget(const std::string& p_name) :
        spk::IWidget(p_name),
        _backgroundTilemapTexture("Playground/worldBackground.png", spk::Vector2Int(8, 6)),
        _playerSpriteSheet("Playground/playerSpriteSheet.png", spk::Vector2Int(4, 4)),
        _gameEngineManager("GameEngineManager", this),
        _playerObject("Player"),
        _playerBodyRenderer(_playerObject.addComponent<spk::MeshRenderer>("BodyRenderer")),
        _cameraObject("Camera", &_playerObject),
        _cameraComponent(_cameraObject.addComponent<spk::Camera>("Camera")), 
        _backgroundTilemap("BackgroundTilemap"),
        _tilemapComponent(_backgroundTilemap.addComponent<Tilemap2D>("Tilemap"))
    {
        _playerObject.transform().translation = spk::Vector3(2, 0, 5);
        _playerBodyRenderer->setSpriteSheet(&_playerSpriteSheet);  
        _playerBodyRenderer->setSprite(spk::Vector2Int(0, 0));

        _cameraObject.transform().translation = spk::Vector3(0, 0, 10);  
        _cameraObject.transform().lookAt(spk::Vector3(0, 0, 0));
        _cameraComponent->setAsMainCamera();
        _cameraComponent->setType(spk::Camera::Type::Orthographic);
        _cameraComponent->setOrthographicSize(10);

        _backgroundTilemap.transform().translation = spk::Vector3(0, 0, 0);
        Tilemap2D::Chunk::texture = &_backgroundTilemapTexture;

        _tilemapComponent->createEmpyChunk(spk::Vector2Int(0, 0))->validate();
        _tilemapComponent->createEmpyChunk(spk::Vector2Int(1, 0))->validate();
        _tilemapComponent->createEmpyChunk(spk::Vector2Int(0, 1))->validate();
        _tilemapComponent->createEmpyChunk(spk::Vector2Int(1, 1))->validate();
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