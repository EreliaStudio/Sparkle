#include "map_manager.hpp"

#include "event.hpp"

std::string MapManager::_composeChunkFileName(const spk::Vector2Int& p_chunkPosition)
{
	return ("Chunk_" + std::to_string(p_chunkPosition.x) + "_" + std::to_string(p_chunkPosition.y) + ".chunk");
}

void MapManager::_loadMap()
{
	_chunkEntities.clear();
	for (auto& chunk : _chunkEntities)
	{
		std::filesystem::path chunkFilePath = _worldFolderPath / _composeChunkFileName(chunk.first);
		chunk.second->load(chunkFilePath);
	}
}

void MapManager::_saveMap()
{
	for (auto& chunk : _chunkEntities)
	{
		std::filesystem::path chunkFilePath = _worldFolderPath / _composeChunkFileName(chunk.first);
		chunk.second->save(chunkFilePath);
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

void MapManager::configure(const spk::JSON::File& p_configurationFile)
{
	setWorldFolder(p_configurationFile[L"worldFolder"].as<std::wstring>());
}

void MapManager::setWorldFolder(const std::filesystem::path& p_worldPath)
{
	_worldFolderPath = p_worldPath;
}

void MapManager::setNode(spk::Vector2Int p_nodePosition, int p_layer, int p_nodeID)
{
	spk::Vector2Int chunkPosition = Chunk::convertWorldToChunkPosition(p_nodePosition);

	spk::SafePointer<ChunkEntity> targetChunkEntity = chunkEntity(chunkPosition);

	spk::SafePointer<BakableChunk> targetChunk = spk::safe_pointer_cast<BakableChunk>(targetChunkEntity->chunk());

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
	return (_chunkEntities[p_chunkPosition].get());
}