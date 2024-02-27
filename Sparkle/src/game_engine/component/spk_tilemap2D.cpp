#include "game_engine/component/spk_tilemap2D.hpp"
#include "game_engine/component/spk_camera_component.hpp"

namespace spk
{
    Tilemap2D::IChunk* Tilemap2D::_insertChunk(spk::GameObject* p_object, const spk::Vector2Int& p_chunkPosition)
    {
        Chunk* result = p_object->addComponent<Chunk>(this, p_chunkPosition);

        result->setSpriteSheet(_spriteSheet);

        return (result);
    }

    Tilemap2D::Tilemap2D(const std::string& p_name) :
        ITilemap(p_name)
    {
    }

    void Tilemap2D::setSpriteSheet(const spk::SpriteSheet* p_spriteSheet)
    {
        _spriteSheet = p_spriteSheet;
        for (auto& [key, element] : chunksObjects())
        {
            element->getComponent<Chunk>()->setSpriteSheet(_spriteSheet);
        }
    }

    bool Tilemap2D::containsNode(const typename IChunk::NodeIndexType& p_nodeIndex) const
    {
        return (_nodes.contains(p_nodeIndex));
    }

    void Tilemap2D::insertNodeType(const typename IChunk::NodeIndexType& p_nodeIndex, const Tilemap2D::Node& p_node)
    {
        _nodes[p_nodeIndex] = p_node;
    }

    const Tilemap2D::Node& Tilemap2D::node(const typename IChunk::NodeIndexType& p_nodeIndex) const
    {
        return (_nodes.at(p_nodeIndex));
    }

	void Tilemap2D::setActiveChunkRange(const spk::Vector2Int& p_activeChunkStart, const spk::Vector2Int& p_activeChunkEnd)
	{
		_activeChunkStart = p_activeChunkStart;
		_activeChunkEnd = p_activeChunkEnd;
	}

    std::vector<spk::Vector2Int> Tilemap2D::missingChunks() const
    {
        std::vector<spk::Vector2Int> result;

        for (int x = _activeChunkStart.x; x <= _activeChunkEnd.x; x++)
        {
            for (int y = _activeChunkStart.y; y <= _activeChunkEnd.y; y++)
            {
                spk::Vector2Int chunkPosition = spk::Vector2Int(x, y);
                if (containsChunk(chunkPosition) == false)
                    result.push_back(chunkPosition);
            }
        }

        return (result);
    }

    void Tilemap2D::updateActiveChunks()
    {
        std::vector<spk::GameObject*> chunksToActivate;

        for (int x = _activeChunkStart.x; x <= _activeChunkEnd.x; x++)
        {
            for (int y = _activeChunkStart.y; y <= _activeChunkEnd.y; y++)
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
}