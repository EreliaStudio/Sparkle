#pragma once

#include "sparkle.hpp"
#include "structure/engine/spk_component.hpp"
#include "chunk_entity.hpp"
#include <unordered_map>
#include <memory>

class MapManager : public spk::Component
{
private:
	std::filesystem::path _worldFolderPath;

	spk::ContractProvider::Contract _loadMapContract;
	spk::ContractProvider::Contract _saveMapContract;

	std::unordered_map<spk::Vector2Int, std::unique_ptr<ChunkEntity>> _chunkEntities;
	
	static std::string _composeChunkFileName(const spk::Vector2Int& p_chunkPosition);

	void _loadMap();
	void _saveMap();

public:
	MapManager(const std::wstring& p_name);

	void configure(const spk::JSON::File& p_configurationFile);

	void setWorldFolder(const std::filesystem::path& p_worldPath);
	void loadWorld(const std::filesystem::path& p_worldPath);

	void setNode(spk::Vector2Int p_nodePosition, int p_layer, int p_nodeID);
	spk::SafePointer<ChunkEntity> chunkEntity(const spk::Vector2Int& p_chunkPosition);
};