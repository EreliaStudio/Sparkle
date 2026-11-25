#pragma once

#include <concepts>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "math/vector2.hpp"
#include "world/generator.hpp"

namespace spk::World
{
	// A surface pass operates only on a horizontal coordinate and construction data.
	template <typename TSurfaceData>
	class SurfacePass
	{
	public:
		virtual ~SurfacePass() = default;
		virtual void apply(const spk::Vector2Int &coordinate, TSurfaceData &data) = 0;
	};

	// Owns the surface construction pipeline and publishes one immutable result per
	// column. Interest and job reservations are independent reference counts.
	template <typename TChunk, typename TGenerationData>
	class SurfaceGenerationPass : public Generator<TChunk, TGenerationData>::Pass
	{
	public:
		using SurfaceDataType = typename TGenerationData::Surface;
		using Pass = SurfacePass<SurfaceDataType>;
		using Handle = std::shared_ptr<const SurfaceDataType>;

	private:
		struct Entry
		{
			std::size_t interests = 0;
			std::size_t jobs = 0;
			std::mutex mutex;
			Handle result;
		};

		std::mutex _mutex;
		std::map<spk::Vector2Int, std::shared_ptr<Entry>> _entries;
		std::vector<std::unique_ptr<Pass>> _passes;

		[[nodiscard]] static spk::Vector2Int _key(const ChunkCoordinate &coordinate)
		{
			return {coordinate.x, coordinate.z};
		}

		void _retain(const ChunkCoordinate &coordinate, std::size_t Entry::*counter)
		{
			const std::scoped_lock lock(_mutex);
			const auto key = _key(coordinate);
			auto it = _entries.find(key);
			if (it == _entries.end())
			{
				it = _entries.emplace(key, std::make_shared<Entry>()).first;
			}
			auto &count = it->second.get()->*counter;
			if (count == std::numeric_limits<std::size_t>::max())
			{
				throw spk::Exception("Surface generation reference count exhausted");
			}
			++count;
		}

		void _release(const ChunkCoordinate &coordinate, std::size_t Entry::*counter) noexcept
		{
			std::shared_ptr<Entry> retired;
			{
				const std::scoped_lock lock(_mutex);
				auto it = _entries.find(_key(coordinate));
				if (it == _entries.end() || (it->second.get()->*counter) == 0)
				{
					std::terminate();
				}
				--(it->second.get()->*counter);
				if (it->second->jobs == 0 && it->second->interests == 0)
				{
					retired = std::move(it->second);
					_entries.erase(it);
				}
			}
		}

	protected:
		virtual SurfaceDataType _createSurfaceData()
		{
			return {};
		}
		virtual void _finalizeSurfaceData(const spk::Vector2Int &, SurfaceDataType &)
		{
		}
		virtual Handle &_surfaceDestination(TGenerationData &data) = 0;

	public:
		template <typename TPass, typename... TArgs>
			requires std::derived_from<TPass, Pass>
		TPass &addPass(TArgs &&...args)
		{
			auto pass = std::make_unique<TPass>(std::forward<TArgs>(args)...);
			auto &result = *pass;
			_passes.push_back(std::move(pass));
			return result;
		}

		void retain(const ChunkCoordinate &coordinate) override
		{
			_retain(coordinate, &Entry::interests);
		}
		void release(const ChunkCoordinate &coordinate) noexcept override
		{
			_release(coordinate, &Entry::interests);
		}
		void beginJob(const ChunkCoordinate &coordinate) override
		{
			_retain(coordinate, &Entry::jobs);
		}
		void endJob(const ChunkCoordinate &coordinate) noexcept override
		{
			_release(coordinate, &Entry::jobs);
		}

		void apply(const ChunkCoordinate &coordinate, TGenerationData &data) override
		{
			std::shared_ptr<Entry> entry;
			const auto key = _key(coordinate);
			{
				const std::scoped_lock lock(_mutex);
				entry = _entries.at(key);
			}
			Handle result;
			{
				// Only jobs for this column wait. Failed builds publish nothing;
				// the next caller may retry.
				const std::scoped_lock lock(entry->mutex);
				if (!entry->result)
				{
					auto building = std::make_shared<SurfaceDataType>(_createSurfaceData());
					for (auto &pass : _passes)
					{
						pass->apply(key, *building);
					}
					_finalizeSurfaceData(key, *building);
					entry->result = std::move(building);
				}
				result = entry->result;
			}
			_surfaceDestination(data) = std::move(result);
		}
	};
}
