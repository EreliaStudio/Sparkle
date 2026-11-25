#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "math/vector2.hpp"

namespace spk::World
{
	// Persistent column storage shared by chunks with the same X/Z coordinates.
	template <typename TElement>
	class ColumnGenerationDataCollection
	{
	public:
		using Handle = std::shared_ptr<const TElement>;

	private:
		struct Entry
		{
			std::mutex mutex;
			Handle result;
		};

		mutable std::mutex _mutex;
		std::map<spk::Vector2Int, std::shared_ptr<Entry>> _entries;

	public:
		// Build and publish a complete immutable column, or leave it retryable.
		// The callback must not recursively request columns from this cache.
		template <typename TBuild>
		[[nodiscard]] Handle obtain(const spk::Vector2Int &coordinate, TBuild &&build)
		{
			std::shared_ptr<Entry> entry;
			{
				const std::scoped_lock lock(_mutex);
				auto it = _entries.find(coordinate);
				if (it == _entries.end())
				{
					it = _entries.emplace(coordinate, std::make_shared<Entry>()).first;
				}
				entry = it->second;
			}
			const std::scoped_lock lock(entry->mutex);
			if (!entry->result)
			{
				auto column = std::make_shared<TElement>();
				build(*column);
				entry->result = std::move(column);
			}
			return entry->result;
		}

		[[nodiscard]] std::size_t count() const
		{
			const std::scoped_lock lock(_mutex);
			return _entries.size();
		}

		// Reclaim idle columns only; active builds and externally held handles
		// keep their entries alive. Queued jobs do not need reservations.
		void flush()
		{
			std::vector<std::shared_ptr<Entry>> retired;
			{
				const std::scoped_lock lock(_mutex);
				for (auto it = _entries.begin(); it != _entries.end();)
				{
					const auto &entry = it->second;
					if (entry.use_count() == 1 && (!entry->result || entry->result.use_count() == 1))
					{
						retired.push_back(std::move(it->second));
						it = _entries.erase(it);
					}
					else
					{
						++it;
					}
				}
			}
		}
	};
}
