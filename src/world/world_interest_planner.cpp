#include "world/detail/world_interest_planner.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "engine/registry.hpp"
#include "exception.hpp"
#include "world/world_observer.hpp"

namespace spk::detail
{
	WorldInterestPlanner::WorldInterestPlanner(Vector3Int dimensions, WorldToChunk worldToChunk) :
		_dimensions(dimensions),
		_worldToChunk(worldToChunk),
		_coordinates(PriorityComparator{&_priorities})
	{
		if (_dimensions.x <= 0 || _dimensions.y <= 0 || _dimensions.z <= 0 || _worldToChunk == nullptr)
		{
			throw spk::Exception("World interest planning requires positive chunk dimensions and a conversion function");
		}
	}

	WorldInterestPlanner::Observers WorldInterestPlanner::_capture(EngineHeadless *engine) const
	{
		Observers observers;
		const auto &registered = Registry<WorldObserver, EngineHeadless *>::instance().elements(engine);
		for (WorldObserver *observer : registered)
		{
			if (observer->isEffectivelyActive())
			{
				observers.emplace(observer, ObserverState{_worldToChunk(observer->worldPosition()), observer->interest()});
			}
		}
		return observers;
	}

	std::uint64_t WorldInterestPlanner::_distance(const ChunkCoordinate &coordinate, const ChunkCoordinate &center) const
	{
		std::uint64_t total = 0;
		const auto addAxis = [&total](std::int32_t value, std::int32_t origin, std::int32_t dimension) {
			const std::int64_t delta = static_cast<std::int64_t>(value) - origin;
			const std::uint64_t magnitude = static_cast<std::uint64_t>(delta < 0 ? -delta : delta);
			const std::uint64_t contribution = magnitude * static_cast<std::uint64_t>(dimension);
			const std::uint64_t remaining = std::numeric_limits<std::uint64_t>::max() - total;
			total += std::min(contribution, remaining);
		};

		addAxis(coordinate.x, center.x, _dimensions.x);
		addAxis(coordinate.y, center.y, _dimensions.y);
		addAxis(coordinate.z, center.z, _dimensions.z);
		return total;
	}

	void WorldInterestPlanner::_rebuild(const Observers &observers)
	{
		Priorities priorities;
		for (const auto &[observer, state] : observers)
		{
			(void)observer;
			for (const ChunkCoordinate &coordinate : state.interest.coordinates(state.center))
			{
				const std::uint64_t priority = _distance(coordinate, state.center);
				auto [it, inserted] = priorities.try_emplace(coordinate, priority);
				if (!inserted)
				{
					it->second = std::min(it->second, priority);
				}
			}
		}

		std::vector<ChunkCoordinate> coordinates;
		coordinates.reserve(priorities.size());
		for (const auto &[coordinate, priority] : priorities)
		{
			(void)priority;
			coordinates.push_back(coordinate);
		}

		_priorities = std::move(priorities);
		_coordinates.clear();
		_coordinates.pushBack(std::move(coordinates));
	}

	bool WorldInterestPlanner::refresh(EngineHeadless *engine)
	{
		Observers observers = _capture(engine);
		if (_hasPlan && observers == _observers)
		{
			return false;
		}

		_rebuild(observers);
		_observers = std::move(observers);
		_hasPlan = true;
		++_revision;
		return true;
	}

	void WorldInterestPlanner::reset()
	{
		_observers.clear();
		_coordinates.clear();
		_priorities.clear();
		_hasPlan = false;
	}

	bool WorldInterestPlanner::contains(const ChunkCoordinate &coordinate) const
	{
		return _priorities.contains(coordinate);
	}

	const PriorityQueue<ChunkCoordinate, WorldInterestPlanner::PriorityComparator> &WorldInterestPlanner::coordinates() const noexcept
	{
		return _coordinates;
	}

	std::uint64_t WorldInterestPlanner::revision() const noexcept
	{
		return _revision;
	}
}
