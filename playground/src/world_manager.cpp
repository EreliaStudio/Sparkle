#include "world_manager.hpp"

#include "camera_manager.hpp"

#include "context.hpp"

void WorldManager::_updateChunkVisibility()
{
	if (Context::instance() == nullptr)
		return ;

	spk::Vector2Int currentCameraChunk = Chunk::convertWorldToChunkPosition(Context::instance()->cameraEntity.transform().position());

	std::vector<spk::SafePointer<ChunkEntity>> chunkToActivate;

	spk::Matrix4x4 inverseMatrix = Context::instance()->mainCamera.inverseProjectionMatrix();

	spk::Vector3 downLeftWorld = inverseMatrix * spk::Vector3(-1, 1, 0);
	spk::Vector3 topRightWorld = inverseMatrix * spk::Vector3(1, -1, 0);

	spk::Vector2Int downLeftChunk = Chunk::convertWorldToChunkPosition(downLeftWorld) + currentCameraChunk - 1;
	spk::Vector2Int topRightChunk = Chunk::convertWorldToChunkPosition(topRightWorld) + currentCameraChunk + 1;

	for (int x = downLeftChunk.x; x <= topRightChunk.x; x++)
	{
		for (int y = downLeftChunk.y; y <= topRightChunk.y; y++)
		{
			chunkToActivate.push_back(Context::instance()->mapManager.chunkEntity(spk::Vector2Int(x, y)));
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

WorldManager::WorldManager(const std::wstring& p_name) :
	spk::Component(p_name),
	_systemInfo(BufferObjectCollection::instance()->UBO("systemInfo")),
	_updateChunkVisibilityContract(EventCenter::instance()->subscribe(Event::UpdateChunkVisibility, [&](){
		_updateChunkVisibility();
	}))
{
	
}

void WorldManager::awake()
{
	EventCenter::instance()->notifyEvent(Event::UpdateChunkVisibility);
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