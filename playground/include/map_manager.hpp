#pragma once

#include "structure/engine/spk_component.hpp"
#include "chunk_entity.hpp"
#include <unordered_map>
#include <memory>

class MapManager : public spk::Component
{
private:
	spk::ContractProvider::Contract _loadMapContract;
	spk::ContractProvider::Contract _saveMapContract;

	std::unordered_map<spk::Vector2Int, std::unique_ptr<ChunkEntity>> _chunkEntities;
	
	void _loadMap();
	void _saveMap();

public:
	MapManager(const std::wstring& p_name);

	void setNode(spk::Vector2Int p_nodePosition, int p_layer, int p_nodeID);
	spk::SafePointer<ChunkEntity> chunkEntity(const spk::Vector2Int& p_chunkPosition);
};