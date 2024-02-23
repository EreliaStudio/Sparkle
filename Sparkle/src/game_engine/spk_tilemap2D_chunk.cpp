#include "game_engine/spk_tilemap2D.hpp"
#include "game_engine/component/spk_camera_component.hpp"

namespace spk
{
    const Tilemap2D::Chunk* Tilemap2D::Chunk::_requestNeightbour(const spk::Vector2Int& p_neightbourOffset)
    {
        if (_neightbours[p_neightbourOffset.x + 1][p_neightbourOffset.y + 1] == nullptr)
        {
            const spk::GameObject* chunkObject = _origin->chunkObject(_position + p_neightbourOffset);
            const Chunk* tmpChunk = (chunkObject == nullptr ? nullptr : chunkObject->getComponent<Chunk>());
            _neightbours[p_neightbourOffset.x + 1][p_neightbourOffset.y + 1] = tmpChunk;
        }

        return (_neightbours[p_neightbourOffset.x + 1][p_neightbourOffset.y + 1]);
    }

    void Tilemap2D::Chunk::_insertNodeData(
        std::vector<VertexData>& p_vertexData,
        std::vector<unsigned int>& p_indexes,
        const spk::Vector3& p_position,
        const spk::Vector3& p_size,
        const spk::Vector2& p_spritePosition,
        const spk::Vector2& p_spriteSize,
        const spk::Vector2& p_animationOffset,
        int p_animationLenght,
        int p_animationDuration)
    {
        size_t previousNbVertices = p_vertexData.size();

        static const spk::Vector3 offsets[4] = {
            spk::Vector3(0, 1, 0),
            spk::Vector3(0, 0, 0),
            spk::Vector3(1, 1, 0),
            spk::Vector3(1, 0, 0)};
        static const spk::Vector2 spriteOffsets[4] = {
            spk::Vector2(0, 0),
            spk::Vector2(0, 1),
            spk::Vector2(1, 0),
            spk::Vector2(1, 1)};
        static const unsigned int indexes[6] = {0, 1, 2, 2, 1, 3};

        for (size_t i = 0; i < 4; i++)
        {
            p_vertexData.push_back(VertexData(
                p_position + p_size * offsets[i],
                p_spritePosition + p_spriteSize * spriteOffsets[i],
                p_animationOffset,
                p_animationLenght,
                p_animationDuration));
        }

        for (size_t i = 0; i < 6; i++)
        {
            p_indexes.push_back(indexes[i] + previousNbVertices);
        }
    }

    void Tilemap2D::Chunk::_insertSimpleNode(std::vector<ITilemap::IChunk::VertexData>& p_vertexData, std::vector<unsigned int>& p_indexes, const Node& p_node, int p_x, int p_y, int p_z)
    {
        _insertNodeData(p_vertexData, p_indexes,
                        spk::Vector3(static_cast<float>(p_x) - 0.5f, static_cast<float>(p_y) - 0.5f, static_cast<float>(p_z)),
                        spk::Vector3(1, 1, 0),
                        static_cast<spk::SpriteSheet*>(Chunk::texture())->sprite(p_node.sprite),
                        static_cast<spk::SpriteSheet*>(Chunk::texture())->unit(),
                        static_cast<spk::Vector2>(p_node.animationFrameOffset) * static_cast<spk::SpriteSheet*>(Chunk::texture())->unit(), p_node.nbFrame, p_node.animationDuration);
    }

    const Tilemap2D::Chunk* Tilemap2D::Chunk::_getNeightbourChunk(spk::Vector3Int& p_relativePosition)
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

    void Tilemap2D::Chunk::_bakeTile(std::vector<VertexData>& p_vertexData, std::vector<unsigned int>& p_indexes, const NodeIndexType& p_index, int p_x, int p_y, int p_z)
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

    void Tilemap2D::Chunk::_bake(std::vector<VertexData>& p_vertexData, std::vector<unsigned int>& p_indexes)
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

    Tilemap2D::Chunk::Chunk(Tilemap2D* p_origin, const spk::Vector2Int& p_chunkPosition) :
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

        setContent(2, 3, 1, 2);
        setContent(5, 6, 1, 3);
        setContent(8, 11, 1, 4);
    }
}