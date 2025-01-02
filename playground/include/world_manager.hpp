#pragma once

#include "structure/engine/spk_component.hpp"

#include "event.hpp"
#include "buffer_object_collection.hpp"

#include "structure/engine/spk_camera.hpp"

#include "chunk_entity.hpp"

struct WorldManager : public spk::Component
{
private:
	EventCenter::Instanciator _eventCenterInstanciator;
	BufferObjectCollection::Instanciator _bindingPointInstanciator;
	spk::OpenGL::UniformBufferObject& _systemInfo;

	spk::ContractProvider::Contract _updateChunkVisibilityContract;
	spk::ContractProvider::Contract _loadMapContract;
	spk::ContractProvider::Contract _saveMapContract;

	spk::Vector2Int _halfSize;

	spk::SafePointer<const spk::Entity> _cameraEntity = nullptr;
	spk::SafePointer<const spk::Camera> _camera = nullptr;

	std::unordered_map<spk::Vector2Int, std::unique_ptr<ChunkEntity>> _chunkEntities;
	std::vector<spk::SafePointer<ChunkEntity>> _activeChunkList;

	void _updateChunkVisibility();
	
	void _loadMap();
	void _saveMap();

public:
	WorldManager(const std::wstring& p_name);

	void awake() override;

	void setCamera(spk::SafePointer<const spk::Entity> p_camera);

	void onUpdateEvent(spk::UpdateEvent& p_event) override;
	void onPaintEvent(spk::PaintEvent& p_event) override;
};