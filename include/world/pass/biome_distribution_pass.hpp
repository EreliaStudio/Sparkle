#pragma once

#include <array>
#include <cmath>
#include <cstddef>

#include "exception.hpp"
#include "world/generator.hpp"

namespace spk::World
{
	class BiomeDistribution
	{
	protected:
		// Validate the entire cell before changing any weight. The order and
		// identity of all slots, including zero-weight slots, are preserved.
		template <typename TInfluence, std::size_t TCapacity>
		static void _normalize(std::array<TInfluence, TCapacity> &cell)
		{
			static_assert(TCapacity > 0);
			double sum = 0.0;
			for (std::size_t index = 0; index < TCapacity; ++index)
			{
				const auto &influence = cell[index];
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
					if (cell[previous].biome == influence.biome)
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
			for (auto &influence : cell)
			{
				if (influence.biome != nullptr)
				{
					influence.weight = static_cast<float>(static_cast<double>(influence.weight) / sum);
				}
			}
		}
	};

	template <typename TChunk, typename TGenerationData>
	class BiomeDistributionPass : public Generator<TChunk, TGenerationData>::Pass, protected BiomeDistribution
	{
	};
}
