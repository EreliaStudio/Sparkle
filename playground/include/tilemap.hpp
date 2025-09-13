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
			spk::Vector2Int _lastPlayerPosition;

			EventDispatcher::Contract<> _onRefreshViewContract;
			EventDispatcher::Contract<> _onPlayerMotionContract;

			spk::Vector2Int _computeChunk(const spk::Vector3 &p_worldPos) const;

			void _refreshRange();

		public:
			ChunkSelector(const std::wstring &p_name);

			void start() override;
			void awake() override;
		};

		spk::SafePointer<ChunkSelector> _chunkSelector;

		void _onChunkGeneration(const spk::Vector2Int &p_chunkCoordinate, Chunk &p_chunkToFill) override;

		void _placeArea(const spk::Vector2Int &p_anchor, const spk::Vector2UInt &p_size);

	public:
		using spk::TileMap<16, 16, 4, TileFlag>::TileMap;

		TileMap(const std::wstring &p_name, spk::SafePointer<spk::Entity> p_parent);

		void generate(const spk::Vector2UInt& p_mapSize);
	};
}