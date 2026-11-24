#pragma once

#include <array>
#include "container/capacity_pool.hpp"
#include "world/biome.hpp"
#include "world/surface_grid.hpp"

namespace spk::World
{
	class SurfaceBiome : public Biome
	{
	public:
		struct Influence
		{
			const SurfaceBiome *biome = nullptr;
			float weight = 0.0f;

			template <std::size_t TCapacity>
			class Grid;
		};

		[[nodiscard]] virtual float groundHeight(const spk::Vector2 &worldPosition) const = 0;
	};

	template <std::size_t TCapacity>
	class SurfaceBiome::Influence::Grid : public SurfaceGrid<std::array<Influence, TCapacity>>
	{
		static_assert(TCapacity > 0);
	public:
		using Cell = std::array<Influence, TCapacity>;
		using Base = SurfaceGrid<Cell>;
		using Configuration = typename Base::Configuration;
		using Lease = typename spk::Pool<Grid>::Lease;

	private:
		explicit Grid(std::size_t capacity) : Base(capacity) {}

	public:
		[[nodiscard]] static Lease obtain(const Configuration &configuration, const spk::Vector2 &origin)
		{
			const auto layout = Base::layoutFor(configuration);
			static spk::CapacityPool<Grid> pool([](std::size_t capacity) { return new Grid(capacity); });
			return pool.obtain(layout.count(), [&](Grid &grid) { grid._configure(configuration, origin, layout); });
		}

		[[nodiscard]] const SurfaceBiome *dominant(const spk::Vector2 &position) const
		{
			const SurfaceBiome *result = nullptr;
			float weight = 0.0f;
			for (const auto &influence : this->at(position))
			{
				if (influence.biome != nullptr && influence.weight > weight)
				{
					result = influence.biome;
					weight = influence.weight;
				}
			}
			return result;
		}
	};
}
