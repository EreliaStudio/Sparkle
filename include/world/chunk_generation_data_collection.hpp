#pragma once

#include <map>
#include <mutex>
#include <utility>

#include "exception.hpp"
#include "world/chunk_coordinate.hpp"

namespace spk::World
{
	// Optional storage for request-local data in a persistent generation state.
	// Each active coordinate owns exactly one entry until its generation ends.
	// Concurrent generation of the same coordinate is rejected.
	template <typename TElement>
	class ChunkGenerationDataCollection
	{
	private:
		mutable std::mutex _mutex;
		std::map<ChunkCoordinate, TElement> _entries;

	public:
		template <typename... TArgs>
		TElement &emplace(const ChunkCoordinate &coordinate, TArgs &&...args)
		{
			const std::scoped_lock lock(_mutex);
			auto [it, inserted] = _entries.try_emplace(coordinate, std::forward<TArgs>(args)...);
			if (!inserted)
			{
				throw spk::Exception("Chunk generation is already active for this coordinate");
			}
			return it->second;
		}

		[[nodiscard]] TElement &at(const ChunkCoordinate &coordinate)
		{
			const std::scoped_lock lock(_mutex);
			return _entries.at(coordinate);
		}
		[[nodiscard]] const TElement &at(const ChunkCoordinate &coordinate) const
		{
			const std::scoped_lock lock(_mutex);
			return _entries.at(coordinate);
		}

		void erase(const ChunkCoordinate &coordinate) noexcept
		{
			const std::scoped_lock lock(_mutex);
			_entries.erase(coordinate);
		}

		[[nodiscard]] std::size_t count() const
		{
			const std::scoped_lock lock(_mutex);
			return _entries.size();
		}
	};
}
