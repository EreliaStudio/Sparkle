#pragma once

#include <array>
#include <cmath>
#include <cstddef>

#include "exception.hpp"
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

		// Validate the entire cell before changing any weight. The order and
		// identity of all slots, including zero-weight slots, are preserved.
		void normalize()
		{
			double sum = 0.0;
			for (std::size_t index = 0; index < TCapacity; ++index)
			{
				const auto &influence = elements[index];
				if (influence.biome == nullptr)
				{
					if (influence.weight != 0.0f)
					{
						throw spk::Exception("An empty biome influence must have zero weight");
					}
					continue;
				}
				if (!std::isfinite(influence.weight) || influence.weight < 0.0f)
				{
					throw spk::Exception("Biome influence weight must be finite and nonnegative");
				}
				for (std::size_t previous = 0; previous < index; ++previous)
				{
					if (elements[previous].biome == influence.biome)
					{
						throw spk::Exception("Duplicate biome in influence cell");
					}
				}
				sum += static_cast<double>(influence.weight);
			}

			if (sum == 0.0)
			{
				return;
			}
			for (auto &influence : elements)
			{
				if (influence.biome != nullptr)
				{
					influence.weight = static_cast<float>(static_cast<double>(influence.weight) / sum);
				}
			}
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
		// Four neighboring cells may contain completely different biomes.
		using InterpolatedCell = Influence::Cell<4 * TCapacity>;

	private:
		Grid() = default;

	public:
		[[nodiscard]] static Lease obtain(const Configuration &configuration, const spk::Vector2 &origin)
		{
			return Base::template _obtain<Grid>(configuration, origin, []() {
				return new Grid();
			},
												[](Grid &) {
												});
		}

		[[nodiscard]] InterpolatedCell interpolate(const spk::Vector2 &position) const
		{
			InterpolatedCell result{};
			std::array<double, 4 * TCapacity> weights{};
			std::size_t count = 0;
			const auto accumulate = [&](const Cell &cell, double factor) {
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
			this->_forEachInterpolationSample(position, accumulate);
			for (std::size_t i = 0; i < count; ++i)
			{
				result[i].weight = static_cast<float>(weights[i]);
			}
			return result;
		}
	};
}
