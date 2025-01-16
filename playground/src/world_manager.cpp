#include "world_manager.hpp"

#include "camera_manager.hpp"

void WorldManager::_updateChunkVisibility()
{
	if (_camera == nullptr || _cameraEntity == nullptr)
	{
		return;
	}

	spk::Vector2Int currentCameraChunk = Chunk::convertWorldToChunkPosition(_cameraEntity->transform().position());

	std::vector<spk::SafePointer<ChunkEntity>> chunkToActivate;

	spk::Matrix4x4 inverseMatrix = _camera->inverseProjectionMatrix();

	spk::Vector3 downLeftWorld = inverseMatrix * spk::Vector3(-1, 1, 0);
	spk::Vector3 topRightWorld = inverseMatrix * spk::Vector3(1, -1, 0);

	spk::Vector2Int downLeftChunk = Chunk::convertWorldToChunkPosition(downLeftWorld) + currentCameraChunk - 1;
	spk::Vector2Int topRightChunk = Chunk::convertWorldToChunkPosition(topRightWorld) + currentCameraChunk + 1;

	for (int x = downLeftChunk.x; x <= topRightChunk.x; x++)
	{
		for (int y = downLeftChunk.y; y <= topRightChunk.y; y++)
		{
			chunkToActivate.push_back(chunkEntity(spk::Vector2Int(x, y)));
		}
	}

	for (auto& chunk : _activeChunkList)
	{
		if (std::find(chunkToActivate.begin(), chunkToActivate.end(), chunk) == chunkToActivate.end())
		{
			chunk->deactivate();
		}
	}

	_activeChunkList = chunkToActivate;
	
	for (auto& chunk : _activeChunkList)
	{
		if (chunk->isActive() == false)
		{
			chunk->activate();
		}
	}
}

void WorldManager::_loadMap()
{
	for (auto& chunk : _chunkEntities)
	{
		chunk.second->load();
	}
}

void WorldManager::_saveMap()
{
	for (auto& chunk : _chunkEntities)
	{
		chunk.second->save();
	}
}

WorldManager::WorldManager(const std::wstring& p_name) :
	spk::Component(p_name),
	_systemInfo(BufferObjectCollection::instance()->UBO("systemInfo")),
	_updateChunkVisibilityContract(EventCenter::instance()->subscribe(Event::UpdateChunkVisibility, [&](){
		_updateChunkVisibility();
	})),
	_loadMapContract(EventCenter::instance()->subscribe(Event::LoadMap, [&](){
		_loadMap();
	})),
	_saveMapContract(EventCenter::instance()->subscribe(Event::SaveMap, [&](){
		_saveMap();
	}))
{

}

void WorldManager::awake()
{
	EventCenter::instance()->notifyEvent(Event::UpdateChunkVisibility);
}

void WorldManager::setCamera(spk::SafePointer<const spk::Entity> p_camera)
{
	_cameraEntity = p_camera;
	auto& _CameraManager = _cameraEntity->getComponent<CameraManager>();
	_camera = &(_CameraManager.camera());
	if (_camera == nullptr)
	{
		throw std::runtime_error("Camera component not found");
	}

	EventCenter::instance()->notifyEvent(Event::UpdateChunkVisibility);
}

spk::SafePointer<ChunkEntity> WorldManager::chunkEntity(const spk::Vector2Int& p_chunkPosition)
{
	if (_chunkEntities.contains(p_chunkPosition) == false)
	{
		_chunkEntities.emplace(p_chunkPosition, std::make_unique<ChunkEntity>(p_chunkPosition, owner()));
	}
	return (_chunkEntities[p_chunkPosition]);
}

void WorldManager::invalidateChunk(const spk::Vector2Int& p_chunkPosition)
{
	static_cast<spk::SafePointer<BakableChunk>>(_chunkEntities[p_chunkPosition]->chunk())->invalidate();
}

void WorldManager::onUpdateEvent(spk::UpdateEvent& p_event)
{
	if (p_event.deltaTime.milliseconds != 0)
	{
		_systemInfo["time"] = static_cast<int>(p_event.time.milliseconds);
		_systemInfo.validate();

		p_event.requestPaint();
	}
}

void WorldManager::onPaintEvent(spk::PaintEvent& p_event)
{
	if (p_event.type == spk::PaintEvent::Type::Resize)
	{
		_halfSize = p_event.geometry.size / 2;

		for (auto& chunk : _activeChunkList)
		{
			chunk->deactivate();
		}
		_activeChunkList.clear();
		EventCenter::instance()->notifyEvent(Event::UpdateChunkVisibility);
	}
}