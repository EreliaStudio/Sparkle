#pragma once

#include <map>
#include <mutex>
#include <thread>
#include <utility>

#include "exception.hpp"
#include "world/chunk_coordinate.hpp"

namespace spk::World
{
	// Optional storage for request-local data in a persistent generation state.
	// A synchronous generation job owns its entry on the calling thread.
	// Duplicate coordinates on different workers remain independent.
	template <typename TElement>
	class ChunkGenerationDataCollection
	{
	private:
		using Key = std::pair<ChunkCoordinate, std::thread::id>;
		mutable std::mutex _mutex;
		std::map<Key, TElement> _entries;

		[[nodiscard]] static Key _key(const ChunkCoordinate &coordinate)
		{
			return {coordinate, std::this_thread::get_id()};
		}

	public:
		template <typename... TArgs>
		TElement &emplace(const ChunkCoordinate &coordinate, TArgs &&...args)
		{
			const std::scoped_lock lock(_mutex);
			auto [it, inserted] = _entries.try_emplace(_key(coordinate), std::forward<TArgs>(args)...);
			if (!inserted)
			{
				throw spk::Exception("Chunk generation is already active on this thread");
			}
			return it->second;
		}

		[[nodiscard]] TElement &at(const ChunkCoordinate &coordinate)
		{
			const std::scoped_lock lock(_mutex);
			return _entries.at(_key(coordinate));
		}
		[[nodiscard]] const TElement &at(const ChunkCoordinate &coordinate) const
		{
			const std::scoped_lock lock(_mutex);
			return _entries.at(_key(coordinate));
		}

		void erase(const ChunkCoordinate &coordinate) noexcept
		{
			const std::scoped_lock lock(_mutex);
			_entries.erase(_key(coordinate));
		}

		[[nodiscard]] std::size_t count() const
		{
			const std::scoped_lock lock(_mutex);
			return _entries.size();
		}
	};
}
