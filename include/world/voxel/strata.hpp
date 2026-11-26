#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include "container/capacity_pool.hpp"
#include "container/grid_2d.hpp"
#include "world/voxel/biome.hpp"

namespace spk::World::Voxel
{
	class Strata
	{
	public:
		// Complete, resolved vertical profile for one horizontal world position.
		class Instance : public spk::LifeCycle
		{
		private:
			struct ResolvedLayer
			{
				double bottom;
				std::uint32_t voxelID;
			};

			std::vector<ResolvedLayer> _layers;
			double _surface = 0.0;
			ReleaseContract _releaseContract;

			explicit Instance(std::size_t capacity) :
				_releaseContract(subscribeToRelease([this]() {
					_layers.clear();
					_surface = 0.0;
				}))
			{
				_layers.reserve(capacity);
			}

		public:
			using Lease = spk::CapacityPool<Instance>::Lease;

			Instance(const Instance &) = delete;
			Instance &operator=(const Instance &) = delete;

			~Instance() override
			{
				terminateLifeCycle();
			}

			[[nodiscard]] static Lease obtain(const Biome &biome, const spk::Vector2 &position, double height)
			{
				if (!std::isfinite(height))
				{
					throw spk::Exception("Voxel terrain height must be finite");
				}
				static spk::CapacityPool<Instance> pool([](std::size_t capacity) {
					return new Instance(capacity);
				});
				auto instance = pool.obtain(std::max(std::size_t{1}, biome.strata().size()));
				instance->_surface = std::floor(height);
				double bottom = instance->_surface;
				biome.sampleStrata(position, [&](const Biome::Layer &layer) {
					bottom = layer.thickness ? bottom - *layer.thickness : -std::numeric_limits<double>::infinity();
					instance->_layers.push_back({bottom, layer.voxelID});
				});
				return instance;
			}

			// Height is absolute. Each layer owns its upper endpoint, but not its lower endpoint.
			// ID zero represents air, including below a finite profile.
			[[nodiscard]] std::uint32_t voxelIDAt(double height) const
			{
				if (!std::isfinite(height))
				{
					throw spk::Exception("Voxel terrain height must be finite");
				}
				if (height > _surface)
				{
					return 0;
				}
				for (const auto &layer : _layers)
				{
					if (height > layer.bottom)
					{
						return layer.voxelID;
					}
				}
				return 0;
			}
		};

		using Grid = spk::Grid2D<Instance::Lease>;

		template <typename TChunk, typename TGenerationData, std::size_t TCapacity = 4>
		class GenerationPass;

		template <typename TChunk, typename TGenerationData>
		class ApplicationPass;
	};
}
