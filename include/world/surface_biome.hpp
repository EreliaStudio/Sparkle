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
			struct Cell;

			template <std::size_t TCapacity>
			class Grid;
		};

		[[nodiscard]] virtual float groundHeight(const spk::Vector2 &worldPosition) const = 0;
	};

	template <std::size_t TCapacity>
	struct SurfaceBiome::Influence::Cell
	{
		static_assert(TCapacity > 0);

		std::array<Influence, TCapacity> elements{};

		[[nodiscard]] auto begin() noexcept
		{
			return elements.begin();
		}
		[[nodiscard]] auto begin() const noexcept
		{
			return elements.begin();
		}
		[[nodiscard]] auto end() noexcept
		{
			return elements.end();
		}
		[[nodiscard]] auto end() const noexcept
		{
			return elements.end();
		}
		[[nodiscard]] auto cbegin() const noexcept
		{
			return elements.cbegin();
		}
		[[nodiscard]] auto cend() const noexcept
		{
			return elements.cend();
		}
		[[nodiscard]] constexpr std::size_t size() const noexcept
		{
			return TCapacity;
		}

		[[nodiscard]] Influence &operator[](std::size_t index) noexcept
		{
			return elements[index];
		}
		[[nodiscard]] const Influence &operator[](std::size_t index) const noexcept
		{
			return elements[index];
		}

		[[nodiscard]] const SurfaceBiome *dominant() const noexcept
		{
			const SurfaceBiome *result = nullptr;
			float weight = 0.0f;
			for (const auto &influence : elements)
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

	template <std::size_t TCapacity>
	class SurfaceBiome::Influence::Grid : public SurfaceGrid<SurfaceBiome::Influence::Cell<TCapacity>>
	{
		static_assert(TCapacity > 0);

	public:
		using Cell = Influence::Cell<TCapacity>;
		using Base = SurfaceGrid<Cell>;
		using Configuration = typename Base::Configuration;
		using Lease = typename spk::Pool<Grid>::Lease;
		enum class Interpolation
		{
			Floor,
			Bilinear
		};
		// Four neighboring cells may contain completely different biomes.
		using InterpolatedCell = Influence::Cell<4 * TCapacity>;

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

		[[nodiscard]] InterpolatedCell interpolate(const spk::Vector2 &position, Interpolation interpolation) const
		{
			if (interpolation == Interpolation::Floor)
			{
				InterpolatedCell result{};
				const auto &cell = this->at(position);
				std::copy(cell.begin(), cell.end(), result.begin());
				return result;
			}
			if (interpolation != Interpolation::Bilinear)
			{
				throw spk::Exception("Unknown biome influence interpolation");
			}
			const auto dimensions = this->sampleSize();
			const auto &config = this->configuration();
			const auto alignedAxis = [&](float coordinate, unsigned int extent, std::size_t count) {
				const double index = this->_sampleAxis(coordinate, extent, count);
				const double nearest = std::round(index);
				// Match the float representation produced by localPosition, not an arbitrary epsilon.
				if (coordinate == static_cast<float>(nearest * config.scale))
				{
					return nearest;
				}
				return index;
			};
			const double x = alignedAxis(position.x, config.size.x, dimensions.x);
			const double z = alignedAxis(position.y, config.size.y, dimensions.y);
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
			if (dx == 0.0 && dz == 0.0)
			{
				accumulate(this->sample({x0, z0}), 1.0);
			}
			else if (dx == 0.0)
			{
				accumulate(this->sample({x0, z0}), 1 - dz);
				accumulate(this->sample({x0, z1}), dz);
			}
			else if (dz == 0.0)
			{
				accumulate(this->sample({x0, z0}), 1 - dx);
				accumulate(this->sample({x1, z0}), dx);
			}
			else
			{
				accumulate(this->sample({x0, z0}), (1 - dx) * (1 - dz));
				accumulate(this->sample({x1, z0}), dx * (1 - dz));
				accumulate(this->sample({x0, z1}), (1 - dx) * dz);
				accumulate(this->sample({x1, z1}), dx * dz);
			}
			for (std::size_t i = 0; i < count; ++i)
			{
				result[i].weight = static_cast<float>(weights[i]);
			}
			return result;
		}
	};
}
