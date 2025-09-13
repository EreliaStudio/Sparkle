#include "tilemap.hpp"

namespace taag
{
	void TileMap::ChunkSelector::_refreshRange()
	{
		if (CameraHolder::mainCamera() == nullptr)
		{
			return;
		}

		spk::Vector2Int currentChunk = TileMap::worldToChunkCoordinates(CameraHolder::mainCamera()->transform().position().xy());

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

		_tileMap->activateChunks(start, end);
	}

	TileMap::ChunkSelector::ChunkSelector(const std::wstring &p_name) :
		spk::Component(p_name)
	{
	}

	void TileMap::ChunkSelector::start()
	{
		_tileMap = owner().upCast<TileMap>();
		_lastPlayerPosition = -1;

		_onRefreshViewContract = EventDispatcher::subscribe(
			Event::RefreshView,
			[&]()
			{
				_lastChunk.reset();
				_refreshRange();
			});
		_onPlayerMotionContract = EventDispatcher::subscribe(Event::PlayerMotion, [&]()
		{
			_refreshRange();
		});
	}

	void TileMap::ChunkSelector::awake()
	{
		_refreshRange();
	}

	void TileMap::_onChunkGeneration(const spk::Vector2Int &p_chunkCoordinate, Chunk &p_chunkToFill)
	{
		for (size_t i = 0; i < Chunk::size.x; i++)
		{
			for (size_t j = 0; j < Chunk::size.y; j++)
			{
				p_chunkToFill.setContent(i, j, 0, (p_chunkCoordinate.x + p_chunkCoordinate.y) % 2 == 0 ? 2 : 1);
			}
		}

		auto collisionManager = p_chunkToFill.addComponent<Chunk::Collider>(p_chunkToFill.name() + L"/Collider");
		collisionManager->setFlag(TileFlag::Obstacle);
	}

	TileMap::TileMap(const std::wstring &p_name, spk::SafePointer<spk::Entity> p_parent) :
		spk::TileMap<16, 16, 4, TileFlag>(p_name, p_parent),
		_chunkSelector(addComponent<ChunkSelector>(p_name + L"/ChunkSelector"))
	{

	}

	struct AvailablePool
	{
		static inline const bool Empty = false;
		static inline const bool Full = true;

		spk::Vector2UInt mapSize{};
		std::unordered_map<spk::Vector2UInt, bool> spaceLeft;
		std::vector<spk::Vector2UInt> nodeToTest;
		spk::RandomGenerator<int> randomGenerator;

		AvailablePool(const unsigned int &p_seed, const spk::Vector2UInt& p_mapSize) :
			randomGenerator(p_seed),
			mapSize(p_mapSize)
		{
			const size_t total = static_cast<size_t>(p_mapSize.x) * static_cast<size_t>(p_mapSize.y);
			spaceLeft.reserve(total);
			nodeToTest.reserve(total);
			for (size_t i = 0; i < p_mapSize.x; i++)
			{
				for (size_t j = 0; j < p_mapSize.y; j++)
				{
					spaceLeft[{i, j}] = Empty;
					nodeToTest.push_back({i, j});
				}
			}
			std::shuffle(nodeToTest.begin(), nodeToTest.end(), randomGenerator.generator());
		}

		bool check(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size)
		{
			for (size_t i = 0; i < p_size.x; i++)
			{
				for (size_t j = 0; j < p_size.y; j++)
				{
					spk::Vector2Int tmp = {i, j};
					if (spaceLeft.contains(tmp) == false ||
						spaceLeft.at(tmp) == Full)
					{
						return (false);
					}
				}
			}
			return (true);
		}

		std::optional<spk::Vector2Int> find(const spk::Vector2UInt& p_size)
		{
			for (auto pos : nodeToTest)
			{
				if (check(pos, p_size) == true)
				{
					return (pos);
				}
			}
			return (std::nullopt);
		}

		void insertArea(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size)
		{
			for (int i = 0; i < p_size.x; i++)
			{
				for (int j = 0; j < p_size.y; j++)
				{
					spk::Vector2Int tmp = {i, j};
					spaceLeft[tmp] = Full;
					//Remove the nodeTotest i/j
					nodeToTest.erase(
						std::remove_if(nodeToTest.begin(), nodeToTest.end(),
							[&](const spk::Vector2UInt& c){return c == tmp;}),
						nodeToTest.end());
				}
			}
		}
	};

	void TileMap::_placeArea(const spk::Vector2Int &p_anchor, const spk::Vector2UInt &p_size)
	{
		for (int chunkX = 0; chunkX < p_size.x; chunkX++)
		{
			for (int chunkY = 0; chunkY < p_size.y; chunkY++)
			{
				spk::Vector2Int chunkCoord = spk::Vector2Int(chunkX, chunkY) + p_anchor;
				spk::SafePointer<Chunk> tmpChunk = chunk(chunkCoord);

				for (int i = 0; i < Chunk::size.x; i++)
				{
					for (int j = 0; j < Chunk::size.y; j++)
					{
						if ((chunkX == 0 && i < 2) ||
							(chunkX == (p_size.x - 1) && i > (Chunk::size.x - 3)) ||
							(chunkY == 0 && j < 2) ||
							(chunkY == (p_size.y - 1) && j > (Chunk::size.y - 3)))
						{
							tmpChunk->setContent(i, j, 0, 0);
						}
					}
				}
			}
		}
	}

	void TileMap::generate(const spk::Vector2UInt& p_mapSize)
	{
		// size_t defaultSeed = 123456789;

		// AvailablePool availablePool = AvailablePool(defaultSeed, p_mapSize);

		// spk::Vector2Int colAndRowSize = p_mapSize / 3;
		// spk::Vector2Int offset = (colAndRowSize - spk::Vector2Int(3, 3)) / 2;

		// for (size_t i = 0; i < 3; i++)
		// {
		// 	for (size_t j = 0; j < 3; j++)
		// 	{
		// 		_placeArea(offset + (colAndRowSize * spk::Vector2Int(i, j)), {3, 3});
		// 	}
		// }
	}
}