#pragma once

#include <cstdint>

#include "world/height_map.hpp"
#include "world/pass/surface_pass.hpp"
#include "world/voxel/strata.hpp"

namespace spk::World::Voxel
{
	template <typename TChunk, typename TGenerationData, std::size_t TCapacity>
	class Strata::GenerationPass : public World::SurfacePass<TChunk, TGenerationData>
	{
	public:
		using InfluenceGrid = World::SurfaceBiome::Influence::Grid<TCapacity>;
		using Grid = Strata::Grid;

		enum class Selection
		{
			Dominant,
			Weighted
		};

	private:
		Selection _selection;
		std::uint32_t _seed;

		// Stateless integer mixing: independent of scheduling, chunk height and float precision.
		[[nodiscard]] static std::uint64_t _mix(std::uint64_t value) noexcept
		{
			value += 0x9e3779b97f4a7c15ULL;
			value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
			value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
			return value ^ (value >> 31);
		}

		[[nodiscard]] const World::SurfaceBiome *_selectBiome(const typename InfluenceGrid::InterpolatedCell &cell, std::uint64_t x, std::uint64_t z) const
		{
			if (_selection == Selection::Dominant)
			{
				return cell.dominant();
			}

			double total = 0.0;
			for (const auto &influence : cell)
			{
				if (influence.biome != nullptr && influence.weight > 0.0f)
				{
					total += influence.weight;
				}
			}
			if (total <= 0.0)
			{
				return nullptr;
			}

			const auto hash = _mix(_mix(x) ^ _mix(z ^ 0xd1b54a32d192ed03ULL) ^ _mix(_seed));
			const double target = static_cast<double>(hash >> 11) * (1.0 / 9007199254740992.0) * total;
			double cumulative = 0.0;
			const World::SurfaceBiome *last = nullptr;
			for (const auto &influence : cell)
			{
				if (influence.biome != nullptr && influence.weight > 0.0f)
				{
					last = influence.biome;
					cumulative += influence.weight;
					if (target < cumulative)
					{
						return last;
					}
				}
			}
			return last;
		}

	public:
		explicit GenerationPass(Selection selection = Selection::Dominant, std::uint32_t seed = 0) :
			_selection(selection),
			_seed(seed)
		{
			if (selection != Selection::Dominant && selection != Selection::Weighted)
			{
				throw spk::Exception("Unknown voxel biome selection");
			}
		}

	protected:
		virtual const InfluenceGrid &_influenceGrid(const spk::Vector2Int &coordinate, const TGenerationData &data) = 0;
		virtual const World::HeightMap::Grid &_heightMap(const spk::Vector2Int &coordinate, const TGenerationData &data) = 0;
		virtual void _storeStrataGrid(const spk::Vector2Int &coordinate, TGenerationData &data, Grid::Lease grid) = 0;

		void _applySurface(const spk::Vector2Int &coordinate, TGenerationData &data) override
		{
			const auto &influences = _influenceGrid(coordinate, data);
			const auto &heights = _heightMap(coordinate, data);
			const spk::Vector2UInt size{static_cast<std::uint32_t>(TChunk::Size.x), static_cast<std::uint32_t>(TChunk::Size.z)};
			const auto origin = TChunk::chunkCoordinateToWorld({coordinate.x, 0, coordinate.y});
			const spk::Vector2 worldOrigin{static_cast<float>(origin.x), static_cast<float>(origin.z)};
			if (influences.configuration().size != size || heights.configuration().size != size ||
				influences.worldOrigin() != worldOrigin || heights.worldOrigin() != worldOrigin)
			{
				throw spk::Exception("Voxel surface grids do not match the chunk footprint");
			}

			auto grid = Grid::obtain(size);
			for (auto &&[position, instance] : *grid)
			{
				const spk::Vector2 localPosition{static_cast<float>(position.x), static_cast<float>(position.y)};
				const auto x = static_cast<std::uint64_t>(origin.x) + position.x;
				const auto z = static_cast<std::uint64_t>(origin.z) + position.y;
				const auto *biome = dynamic_cast<const Biome *>(_selectBiome(influences.interpolate(localPosition), x, z));
				if (biome == nullptr)
				{
					throw spk::Exception("Voxel strata require a selected voxel biome");
				}
				instance = Strata::Instance::obtain(*biome, worldOrigin + localPosition, heights.interpolate(localPosition));
			}
			_storeStrataGrid(coordinate, data, std::move(grid));
		}
	};
}
