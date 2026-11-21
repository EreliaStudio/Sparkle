#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include "container/priority_queue.hpp"
#include "math/vector3.hpp"
#include "world/chunk_coordinate.hpp"
#include "world/chunk_interest.hpp"

namespace spk
{
	class EngineHeadless;
	class WorldObserver;

	namespace detail
	{
		class WorldInterestPlanner
		{
		public:
			using WorldToChunk = ChunkCoordinate (*)(const Vector3 &);

		private:
			struct ObserverState
			{
				ChunkCoordinate center;
				ChunkInterest interest;

				[[nodiscard]] bool operator==(const ObserverState &) const = default;
			};

			using Observers = std::map<const WorldObserver *, ObserverState>;
			using Priorities = std::map<ChunkCoordinate, std::uint64_t>;

			struct PriorityComparator
			{
				const Priorities *priorities;

				[[nodiscard]] bool operator()(const ChunkCoordinate &left, const ChunkCoordinate &right) const
				{
					const auto leftPriority = priorities->at(left);
					const auto rightPriority = priorities->at(right);
					return leftPriority == rightPriority ? left < right : leftPriority < rightPriority;
				}
			};

			Vector3Int _dimensions;
			WorldToChunk _worldToChunk;
			Observers _observers;
			Priorities _priorities;
			PriorityQueue<ChunkCoordinate, PriorityComparator> _coordinates;
			bool _hasPlan = false;
			std::uint64_t _revision = 0;

			[[nodiscard]] Observers _capture(EngineHeadless *engine) const;
			[[nodiscard]] std::uint64_t _distance(const ChunkCoordinate &coordinate, const ChunkCoordinate &center) const;
			void _rebuild(const Observers &observers);

		public:
			WorldInterestPlanner(Vector3Int dimensions, WorldToChunk worldToChunk);

			[[nodiscard]] bool refresh(EngineHeadless *engine);
			void reset();

			[[nodiscard]] bool contains(const ChunkCoordinate &coordinate) const;
			[[nodiscard]] const PriorityQueue<ChunkCoordinate, PriorityComparator> &coordinates() const noexcept;
			[[nodiscard]] std::uint64_t revision() const noexcept;
		};
	}
}
