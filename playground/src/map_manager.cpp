#include "map_manager.hpp"

#include "event.hpp"

void MapManager::loadMap()
{
	for (auto& chunk : _chunkEntities)
	{
		chunk.second->load();
	}
}

void MapManager::saveMap()
{
	for (auto& chunk : _chunkEntities)
	{
		chunk.second->save();
	}
}

MapManager::MapManager(const std::wstring& p_name) :
	spk::Component(p_name),
	_loadMapContract(EventCenter::instance()->subscribe(Event::LoadMap, [&](){
		loadMap();
	})),
	_saveMapContract(EventCenter::instance()->subscribe(Event::SaveMap, [&](){
		saveMap();
	}))
{
}

void MapManager::setNode(spk::Vector2Int p_nodePosition, int p_layer, int p_nodeID)
{
	spk::Vector2Int chunkPosition = Chunk::convertWorldToChunkPosition(p_nodePosition);

	spk::SafePointer<ChunkEntity> targetChunkEntity = chunkEntity(chunkPosition);

	spk::SafePointer<BakableChunk> targetChunk = targetChunkEntity->chunk();

	spk::Vector2Int relativePosition = p_nodePosition - chunkPosition * Chunk::Size;

	if (targetChunk->content(relativePosition, p_layer) != p_nodeID)
	{
		targetChunk->setContent(relativePosition, p_layer, p_nodeID);
		targetChunk->invalidate();
	}
	
}

spk::SafePointer<ChunkEntity> MapManager::chunkEntity(const spk::Vector2Int& p_chunkPosition)
{
	if (_chunkEntities.contains(p_chunkPosition) == false)
	{
		_chunkEntities.emplace(p_chunkPosition, std::make_unique<ChunkEntity>(p_chunkPosition, owner()));
	}
	return (_chunkEntities[p_chunkPosition]);
}