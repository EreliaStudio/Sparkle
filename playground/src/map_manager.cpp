#include "map_manager.hpp"

#include "event.hpp"

void MapManager::_loadMap()
{
	for (auto& chunk : _chunkEntities)
	{
		chunk.second->load();
	}
}

void MapManager::_saveMap()
{
	for (auto& chunk : _chunkEntities)
	{
		chunk.second->save();
	}
}

MapManager::MapManager(const std::wstring& p_name) :
	spk::Component(p_name),
	_loadMapContract(EventCenter::instance()->subscribe(Event::LoadMap, [&](){
		_loadMap();
	})),
	_saveMapContract(EventCenter::instance()->subscribe(Event::SaveMap, [&](){
		_saveMap();
	}))
{

}

spk::SafePointer<ChunkEntity> MapManager::chunkEntity(const spk::Vector2Int& p_chunkPosition)
{
	if (_chunkEntities.contains(p_chunkPosition) == false)
	{
		_chunkEntities.emplace(p_chunkPosition, std::make_unique<ChunkEntity>(p_chunkPosition, owner()));
	}
	return (_chunkEntities[p_chunkPosition]);
}

void MapManager::invalidateChunk(const spk::Vector2Int& p_chunkPosition)
{
	static_cast<spk::SafePointer<BakableChunk>>(_chunkEntities[p_chunkPosition]->chunk())->invalidate();
}