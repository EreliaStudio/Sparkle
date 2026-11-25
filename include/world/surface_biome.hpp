#pragma once

#include "world/biome.hpp"
#include "world/surface_grid.hpp"
#include <array>

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
		enum class Interpolation
		{
			Floor,
			Bilinear
		};
		// Four neighboring cells may contain completely different biomes.
		using InterpolatedCell = std::array<Influence, 4 * TCapacity>;

	private:
		Grid() = default;

	public:
		[[nodiscard]] static Lease obtain(const Configuration &configuration, const spk::Vector2 &origin)
		{
			return Base::template _obtain<Grid>(configuration, origin, []() {
				return new Grid();
			}, [](Grid &) {
			});
		}

		[[nodiscard]] InterpolatedCell interpolate(const spk::Vector2 &position) const
		{
			const auto dimensions = this->sampleSize();
			const auto &config = this->configuration();
			const double x = this->_sampleAxis(position.x, config.size.x, dimensions.x);
			const double z = this->_sampleAxis(position.y, config.size.y, dimensions.y);
			const auto x0 = static_cast<std::uint32_t>(x);
			const auto z0 = static_cast<std::uint32_t>(z);
			const auto x1 = std::min<std::uint32_t>(x0 + 1, dimensions.x - 1);
			const auto z1 = std::min<std::uint32_t>(z0 + 1, dimensions.y - 1);
			const double dx = x - x0;
			const double dz = z - z0;
			InterpolatedCell result{};
			std::array<double, 4 * TCapacity> weights{};
			std::size_t count = 0;
			const auto accumulate = [&](const Cell &cell, double factor) {
				if (factor == 0.0)
				{
					return;
				}
				for (const auto &influence : cell)
				{
					if (influence.biome == nullptr || influence.weight <= 0.0f)
					{
						continue;
					}
					std::size_t index = 0;
					while (index < count && result[index].biome != influence.biome)
					{
						++index;
					}
					if (index == count)
					{
						result[count++].biome = influence.biome;
					}
					weights[index] += factor * influence.weight;
				}
			};
			accumulate(this->sample({x0, z0}), (1 - dx) * (1 - dz));
			accumulate(this->sample({x1, z0}), dx * (1 - dz));
			accumulate(this->sample({x0, z1}), (1 - dx) * dz);
			accumulate(this->sample({x1, z1}), dx * dz);
			for (std::size_t i = 0; i < count; ++i)
			{
				result[i].weight = static_cast<float>(weights[i]);
			}
			return result;
		}

		[[nodiscard]] const SurfaceBiome *dominant(const spk::Vector2 &position, Interpolation interpolation) const
		{
			const auto select = [](const auto &cell) {
				const SurfaceBiome *result = nullptr;
				float weight = 0.0f;
				for (const auto &influence : cell)
				{
					if (influence.biome != nullptr && influence.weight > weight)
					{
						result = influence.biome;
						weight = influence.weight;
					}
				}
				return result;
			};
			switch (interpolation)
			{
			case Interpolation::Floor:
				return select(this->at(position));
			case Interpolation::Bilinear:
				return select(interpolate(position));
			}
			throw spk::Exception("Unknown biome influence interpolation");
		}
	};
}
