#pragma once

#include <sparkle.hpp>

#include "event.hpp"

#include "camera_holder.hpp"

namespace taag
{
	enum class TileFlag
	{
		None,
		Obstacle,
		TerritoryBlue,
		TerritoryGreen,
		TerritoryRed
	};

	class TileMap : public spk::TileMap<16, 16, 4, TileFlag>
	{
	private:
		class ChunkSelector : public spk::Component
		{
		private:
			EventDispatcher::Instanciator _eventDispatcherInstanciator;

			spk::SafePointer<TileMap> _tileMap;
			std::optional<spk::Vector2Int> _lastChunk;

			EventDispatcher::Contract<> _onRefreshViewContract;
			EventDispatcher::Contract<> _onPlayerMotionContract;

			spk::Vector2Int _computeChunk(const spk::Vector3 &p_worldPos) const
			{
				const spk::Vector2Int &size = TileMap::Chunk::size;

				int cx = static_cast<int>(std::floor(p_worldPos.x / static_cast<float>(size.x)));
				int cy = static_cast<int>(std::floor(p_worldPos.y / static_cast<float>(size.y)));
				return spk::Vector2Int(cx, cy);
			}

			void _refreshRange()
			{
				if (CameraHolder::mainCamera() == nullptr)
				{
					return;
				}

				spk::Vector2Int currentChunk = _computeChunk(CameraHolder::mainCamera()->transform().position());

				if (_lastChunk.has_value() == true && _lastChunk.value() == currentChunk)
				{
					return;
				}
				_lastChunk = currentChunk;

				spk::Vector2Int start;
				spk::Vector2Int end;

				if (CameraHolder::mainCamera() != nullptr)
				{
					const spk::Camera &cam = CameraHolder::mainCamera()->camera();
					const spk::Matrix4x4 &camModel = CameraHolder::mainCamera()->transform().model();

					auto toWorld = [&](const spk::Vector2 &p_ndc) -> spk::Vector3
					{
						spk::Vector3 inCam = cam.convertScreenToCamera(p_ndc);
						return (camModel * spk::Vector4(inCam, 1.0f)).xyz();
					};

					spk::Vector3 wBL = toWorld(spk::Vector2(-1.0f, -1.0f));
					spk::Vector3 wBR = toWorld(spk::Vector2(1.0f, -1.0f));
					spk::Vector3 wTL = toWorld(spk::Vector2(-1.0f, 1.0f));
					spk::Vector3 wTR = toWorld(spk::Vector2(1.0f, 1.0f));

					float minX = std::min(std::min(wBL.x, wBR.x), std::min(wTL.x, wTR.x));
					float maxX = std::max(std::max(wBL.x, wBR.x), std::max(wTL.x, wTR.x));
					float minY = std::min(std::min(wBL.y, wBR.y), std::min(wTL.y, wTR.y));
					float maxY = std::max(std::max(wBL.y, wBR.y), std::max(wTL.y, wTR.y));

					const spk::Vector2Int &cs = TileMap::Chunk::size;
					start.x = static_cast<int>(std::floor(minX / static_cast<float>(cs.x))) - 2;
					start.y = static_cast<int>(std::floor(minY / static_cast<float>(cs.y))) - 2;
					end.x = static_cast<int>(std::floor(maxX / static_cast<float>(cs.x))) + 1;
					end.y = static_cast<int>(std::floor(maxY / static_cast<float>(cs.y))) + 1;
				}

				_tileMap->setChunkRange(start, end);
			}

		public:
			ChunkSelector(const std::wstring &p_name) :
				spk::Component(p_name)
			{
			}

			void start() override
			{
				_tileMap = owner().upCast<TileMap>();

				_onRefreshViewContract = EventDispatcher::subscribe(Event::RefreshView, [&]() { 
					_lastChunk.reset();
					_refreshRange();
				});
				_onPlayerMotionContract = EventDispatcher::subscribe(Event::PlayerMotion, [&]() { _refreshRange(); });
			}

			void awake() override
			{
				_refreshRange();
			}
		};

		spk::SafePointer<ChunkSelector> _chunkSelector;

		void _onChunkGeneration(const spk::Vector2Int &p_chunkCoordinate, Chunk &p_chunkToFill) override
		{
			for (int i = 0; i < Chunk::size.x; i++)
			{
				for (int j = 0; j < Chunk::size.y; j++)
				{
					if (i == 0 || j == 0)
					{
						p_chunkToFill.setContent(i, j, 0, 0);
					}
					// else
					// {
					// 	p_chunkToFill.setContent(i, j, 0, (rand() % 3) + 1);
					// }
				}
			}

			auto collisionManager = p_chunkToFill.addComponent<Chunk::Collider>(p_chunkToFill.name() + L"/Collider");
			collisionManager->setFlag(TileFlag::Obstacle);
		}

	public:
		using spk::TileMap<16, 16, 4, TileFlag>::TileMap;

		TileMap(const std::wstring &p_name, spk::SafePointer<spk::Entity> p_parent) :
			spk::TileMap<16, 16, 4, TileFlag>(p_name, p_parent),
			_chunkSelector(addComponent<ChunkSelector>(p_name + L"/ChunkSelector"))
		{
		}
	};
}