#include "sparkle.hpp"
#include <iostream>
#include <limits>

struct Node
{
    spk::Vector2Int sprite = 0;
    bool isAutotiled = false;

    Node()
    {

    }

    Node(const spk::Vector2Int& p_sprite, bool p_isAutotiled) :
        sprite(p_sprite),
        isAutotiled(p_isAutotiled)
    {

    }
};

class Tilemap2D : public spk::ITilemap<Node, short, 16, 16, 5>
{
public:
    class Chunk : public Tilemap2D::IChunk
    {
    private:
        const Tilemap2D* _origin = nullptr;
        spk::Vector2Int _position;
        const Chunk* _neightbours[3][3] = {
            {nullptr, nullptr, nullptr},
            {nullptr, nullptr, nullptr},
            {nullptr, nullptr, nullptr}
        };

        const Chunk* _requestNeightbour(const spk::Vector2Int& p_neightbourOffset)
        {
            if (_neightbours[p_neightbourOffset.x + 1][p_neightbourOffset.y + 1] == nullptr)
            {
                const spk::GameObject* chunkObject = _origin->chunkObject(_position + p_neightbourOffset);
                const Chunk* tmpChunk = (chunkObject == nullptr ? nullptr : chunkObject->getComponent<Chunk>());
                _neightbours[p_neightbourOffset.x + 1][p_neightbourOffset.y + 1] = tmpChunk;
            }

            return (_neightbours[p_neightbourOffset.x + 1][p_neightbourOffset.y + 1]);
        }

        void _insertNodeData(std::vector<VertexData>& p_vertexData, std::vector<unsigned int>& p_indexes, const spk::Vector3& p_position, const spk::Vector3& p_size, const spk::Vector2& p_spritePosition, const spk::Vector2& p_spriteSize, const spk::Vector2& p_animationOffset, int p_animationLenght)
        {
            size_t previousNbVertices = p_vertexData.size();

            static const spk::Vector3 offsets[4] = {
                spk::Vector3(0, 1, 0),
                spk::Vector3(0, 0, 0),
                spk::Vector3(1, 1, 0),
                spk::Vector3(1, 0, 0)
            };
            static const spk::Vector2 spriteOffsets[4] = {
                spk::Vector2(0, 0),
                spk::Vector2(0, 1),
                spk::Vector2(1, 0),
                spk::Vector2(1, 1)
            };
            static const unsigned int indexes[6] = {0, 1, 2, 2, 1, 3};

            for (size_t i = 0; i < 4; i++)
            {
                p_vertexData.push_back(VertexData(
                    p_position + p_size * offsets[i],
                    p_spritePosition + p_spriteSize * spriteOffsets[i],
                    p_animationOffset, 
                    p_animationLenght
                ));
            }

            for (size_t i = 0; i < 6; i++)
            {
                p_indexes.push_back(indexes[i] + previousNbVertices);
            }
        }

        void _insertSimpleNode(std::vector<VertexData>& p_vertexData, std::vector<unsigned int>& p_indexes, const Node& p_node, int p_x, int p_y, int p_z)
        {
            _insertNodeData(p_vertexData, p_indexes,
                spk::Vector3(static_cast<float>(p_x) - 0.5f, static_cast<float>(p_y) - 0.5f, static_cast<float>(p_z)),
                spk::Vector3(1, 1, 0),
                static_cast<spk::SpriteSheet*>(Chunk::texture())->sprite(p_node.sprite),
                static_cast<spk::SpriteSheet*>(Chunk::texture())->unit(),
                0, 0);
        }

        const Chunk* _getNeightbourChunk(spk::Vector3Int& p_relativePosition)
        {
            spk::Vector2Int chunkOffset = 0;

            if (p_relativePosition.x < 0)
            {
                chunkOffset.x--;
                p_relativePosition.x += SizeX;
            }
            else if (p_relativePosition.x >= SizeX)
            {
                chunkOffset.x++;
                p_relativePosition.x -= SizeX;
            }

            if (p_relativePosition.y < 0)
            {
                chunkOffset.y--;
                p_relativePosition.y += SizeY;
            }
            else if (p_relativePosition.y >= SizeY)
            {
                chunkOffset.y++;
                p_relativePosition.y -= SizeY;
            }

            return (_requestNeightbour(chunkOffset));
        }

        spk::Vector2Int _computeSpriteOffset(int p_baseIndex, int p_quarterIndex, int p_x, int p_y, int p_z)
        {   
            static const spk::Vector3Int quarterOffset[4][3] = {
                {
                    spk::Vector3Int(-1, 0, 0),
                    spk::Vector3Int(-1, 1, 0),
                    spk::Vector3Int(0, 1, 0),
                },
                {
                    spk::Vector3Int(-1, 0, 0),
                    spk::Vector3Int(-1, -1, 0),
                    spk::Vector3Int(0, -1, 0),
                },
                {
                    spk::Vector3Int(0, -1, 0),
                    spk::Vector3Int(1, -1, 0),
                    spk::Vector3Int(1, 0, 0),
                },
                {
                    spk::Vector3Int(1, 0, 0),
                    spk::Vector3Int(1, 1, 0),
                    spk::Vector3Int(0, 1, 0),
                }
            };

            spk::Vector2Int spriteOffset[4][2][2][2] = {
                /*
                BC.
                A#.
                ...                
                */
                { // Top/Left
                    { // A = false
                        { // B = False
                            spk::Vector2Int(0, 0), //[ ][ ][ ]
                            spk::Vector2Int(0, 3)  //[ ][ ][X]
                        },
                        { // B = true
                            spk::Vector2Int(0, 2), //[ ][X][ ]
                            spk::Vector2Int(0, 3)  //[ ][X][X]
                        }
                    },
                    { // A = true
                        { // B = False
                            spk::Vector2Int(1, 2), //[X][ ][ ]
                            spk::Vector2Int(2, 0)  //[X][ ][X]
                        },
                        { // B = true
                            spk::Vector2Int(1, 2), //[X][X][ ]
                            spk::Vector2Int(1, 3)  //[X][X][X]
                        }
                    }
                },
                /*
                ...
                A#.
                BC.                
                */
                { // Down/Left
                    { // A = false
                        { // B = False
                            spk::Vector2Int(0, 1), //[ ][ ][ ]
                            spk::Vector2Int(0, 4)  //[ ][ ][X]
                        },
                        { // B = true
                            spk::Vector2Int(0, 5), //[ ][X][ ]
                            spk::Vector2Int(0, 4)  //[ ][X][X]
                        }
                    },
                    { // A = true
                        { // B = False
                            spk::Vector2Int(1, 5), //[X][ ][ ]
                            spk::Vector2Int(2, 1)  //[X][ ][X]
                        },
                        { // B = true
                            spk::Vector2Int(1, 5), //[X][X][ ]
                            spk::Vector2Int(1, 4)  //[X][X][X]
                        }
                    }
                },
                /*
                ...
                .#C
                .AB                
                */
                { // Down/Right
                    { // A = false
                        { // B = False
                            spk::Vector2Int(1, 1), //[ ][ ][ ]
                            spk::Vector2Int(2, 5)  //[ ][ ][X]
                        },
                        { // B = true
                            spk::Vector2Int(3, 5), //[ ][X][ ]
                            spk::Vector2Int(2, 5)  //[ ][X][X]
                        }
                    },
                    { // A = true
                        { // B = False
                            spk::Vector2Int(3, 4), //[X][ ][ ]
                            spk::Vector2Int(3, 1)  //[X][ ][X]
                        },
                        { // B = true
                            spk::Vector2Int(3, 4), //[X][X][ ]
                            spk::Vector2Int(2, 4)  //[X][X][X]
                        }
                    }
                },
                /*
                .CB
                .#A
                ...                
                */
                { // Top/Right
                    { // A = false
                        { // B = False
                            spk::Vector2Int(1, 0), //[ ][ ][ ]
                            spk::Vector2Int(3, 3)  //[ ][ ][X]
                        },
                        { // B = true
                            spk::Vector2Int(3, 2), //[ ][X][ ]
                            spk::Vector2Int(3, 3)  //[ ][X][X]
                        }
                    },
                    { // A = true
                        { // B = False
                            spk::Vector2Int(2, 2), //[X][ ][ ]
                            spk::Vector2Int(3, 0)  //[X][ ][X]
                        },
                        { // B = true
                            spk::Vector2Int(2, 2), //[X][X][ ]
                            spk::Vector2Int(2, 3)  //[X][X][X]
                        }
                    }
                }
            };

            bool isSame[3] = {false, false, false};

            for (size_t i = 0; i < 3; i++)
            {
                spk::Vector3Int toCheck = spk::Vector3Int(p_x, p_y, p_z) + quarterOffset[p_quarterIndex][i];

                const Chunk* chunkToCheck = _getNeightbourChunk(toCheck);
                
                if (chunkToCheck != nullptr)
                {
                    if (chunkToCheck->content(toCheck) == p_baseIndex)
                    {
                        isSame[i] = true;
                    }
                }
            }

            return (spriteOffset[p_quarterIndex][isSame[0]][isSame[1]][isSame[2]]);
        }
        
        void _insertAutotiledNode(std::vector<VertexData>& p_vertexData, std::vector<unsigned int>& p_indexes, const NodeIndexType& p_index, const Node& p_node, int p_x, int p_y, int p_z)
        {
            spk::Vector3 offsets[4] = {
                spk::Vector3(0.0f, 0.5f, 0.0f),
                spk::Vector3(0.0f, 0.0f, 0.0f),
                spk::Vector3(0.5f, 0.0f, 0.0f),
                spk::Vector3(0.5f, 0.5f, 0.0f)
            };

            for(size_t i = 0; i < 4; i++)
            {
                _insertNodeData(p_vertexData, p_indexes,
                    spk::Vector3(static_cast<float>(p_x) - 0.5f, static_cast<float>(p_y) - 0.5f, static_cast<float>(p_z)) + offsets[i],
                    spk::Vector3(0.5f, 0.5f, 0),
                    static_cast<spk::SpriteSheet*>(Chunk::texture())->sprite(p_node.sprite + _computeSpriteOffset(p_index, i, p_x, p_y, p_z)),
                    static_cast<spk::SpriteSheet*>(Chunk::texture())->unit(),
                    0, 0);
            }
        }

        void _bakeTile(std::vector<VertexData>& p_vertexData, std::vector<unsigned int>& p_indexes, const NodeIndexType& p_index, int p_x, int p_y, int p_z)
        {
            const Node& tmpNode = _origin->node(p_index);

            if (tmpNode.isAutotiled == true)
            {
                _insertAutotiledNode(p_vertexData, p_indexes, p_index, tmpNode, p_x, p_y, p_z);
            }
            else
            {
                _insertSimpleNode(p_vertexData, p_indexes, tmpNode, p_x, p_y, p_z);
            }
        }

        void _bake(std::vector<VertexData>& p_vertexData, std::vector<unsigned int>& p_indexes) override
        {
            for (int i = 0; i < Tilemap2D::Chunk::SizeX; i++)
            {
                for (int j = 0; j < Tilemap2D::Chunk::SizeY; j++)
                {
                    for (int k = 0; k < Tilemap2D::Chunk::SizeZ; k++)
                    {
                        Tilemap2D::Chunk::NodeIndexType index = content(i, j, k);

                        if (index >= 0 && _origin->containsNode(index) == true)
                        {
                            _bakeTile(p_vertexData, p_indexes, index, i, j, k);
                        }
                    }
                }
            }
        }

    public:
        Chunk(Tilemap2D* p_origin, const spk::Vector2Int& p_chunkPosition) :
            ITilemap::IChunk("Chunk [" + p_chunkPosition.to_string() + "]"),
            _position(p_chunkPosition),
            _origin(p_origin)
        {
            for (int i = -1; i <= 1; i++)
            {
                for (int j = -1; j <= 1; j++)
                {
                    _requestNeightbour(spk::Vector2Int(i, j));
                }
            }

            owner()->transform().translation = spk::Vector3(static_cast<float>(p_chunkPosition.x * Chunk::SizeX), static_cast<float>(p_chunkPosition.y * Chunk::SizeY), 0.0f);
        
            for (size_t i = 0; i < SizeX; i++)
            {
                for (size_t j = 0; j < SizeY; j++)
                {
                    setContent(i, j, 0, (i == 0 || j == 0 ? 0 : 1));
                }
            }
        }
    };

private:
    Chunk* _insertChunk(spk::GameObject* p_object, const spk::Vector2Int& p_chunkPosition) override
    {
        Chunk* result = p_object->addComponent<Chunk>(this, p_chunkPosition); 

        result->setTexture(_spriteSheet);

        return (result);
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
	
    spk::SpriteSheet* _spriteSheet;
    std::map<typename IChunk::NodeIndexType, Node> _nodes;

public:
    Tilemap2D(const std::string& p_name) :
        ITilemap(p_name)
    {

    }

    void setSpriteSheet(spk::SpriteSheet* p_spriteSheet)
    {
        _spriteSheet = p_spriteSheet;
        for (auto& [key, element] : chunksObjects())
        {
            element->getComponent<Chunk>()->setTexture(_spriteSheet);
        }
    }

    bool containsNode(const typename IChunk::NodeIndexType& p_nodeIndex) const
    {
        return (_nodes.contains(p_nodeIndex));
    }

    void insertNodeType(const typename IChunk::NodeIndexType& p_nodeIndex, const Node& p_node)
    {
        _nodes[p_nodeIndex] = p_node;
    }

    const Node& node(const typename IChunk::NodeIndexType& p_nodeIndex) const
    {
        return (_nodes.at(p_nodeIndex));
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
        if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::Space) == spk::InputState::Pressed)
        {
            std::cout << "Position : " << _playerObject.transform().translation.get() << std::endl;
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

        _tilemapComponent->setSpriteSheet(&_backgroundTilemapTexture);
        _tilemapComponent->createEmpyChunk(spk::Vector2Int(0, 0));
        _tilemapComponent->createEmpyChunk(spk::Vector2Int(1, 0));
        _tilemapComponent->createEmpyChunk(spk::Vector2Int(0, 1));
        _tilemapComponent->createEmpyChunk(spk::Vector2Int(1, 1));
        _tilemapComponent->insertNodeType(0, Node(spk::Vector2Int(0, 0), true));
        _tilemapComponent->insertNodeType(1, Node(spk::Vector2Int(4, 0), false));
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
