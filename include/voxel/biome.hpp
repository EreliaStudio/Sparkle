#pragma once

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <vector>

#include "math/perlin.hpp"
#include "voxel/volume.hpp"
#include "world/surface_biome.hpp"

namespace spk::Voxel
{
	class Biome : public World::SurfaceBiome
	{
	public:
		struct Stratum
		{
			struct Material
			{
				std::uint32_t voxelID;
				float weight;
			};

			enum class ThicknessMode
			{
				Finite,
				Infinite
			};
			enum class Distribution
			{
				Weighted,
				Perlin,
				Custom
			};

			ThicknessMode thicknessMode = ThicknessMode::Finite;
			std::uint32_t minimumThickness = 1;
			std::uint32_t maximumThickness = 1;
			spk::Perlin thicknessNoise{};
			std::vector<Material> materials;
			Distribution distribution = Distribution::Weighted;
			std::uint32_t seed = 0;
			spk::Perlin materialNoise{};
			std::function<float(const spk::Vector2 &)> customDistribution;
		};

		using StratumContainer = std::vector<Stratum>;

		struct Layer
		{
			std::optional<std::uint32_t> thickness;
			std::uint32_t voxelID;
		};

	private:
		StratumContainer _strata;

		[[nodiscard]] static std::uint32_t _mix(std::uint32_t value) noexcept
		{
			value ^= value >> 16;
			value *= 0x7feb352du;
			value ^= value >> 15;
			value *= 0x846ca68bu;
			return value ^ (value >> 16);
		}

		[[nodiscard]] static double _weightedSample(const spk::Vector2 &position, std::uint32_t seed)
		{
			// Float positions are the public surface sampling contract.
			const auto x = std::bit_cast<std::uint32_t>(position.x == 0.0f ? 0.0f : position.x);
			const auto z = std::bit_cast<std::uint32_t>(position.y == 0.0f ? 0.0f : position.y);
			return static_cast<double>(_mix(seed ^ _mix(x) ^ _mix(z + 0x9e3779b9u))) / 4294967296.0;
		}

		[[nodiscard]] static Layer _sample(const Stratum &stratum, const spk::Vector2 &position)
		{
			double total = 0.0;
			for (const auto &material : stratum.materials)
			{
				if (material.voxelID == 0 || material.voxelID > Volume::PackedCell::MaxDefinitionId ||
					!std::isfinite(material.weight) || material.weight < 0.0f)
				{
					throw spk::Exception("Invalid voxel biome stratum material");
				}
				total += material.weight;
			}
			if (total <= 0.0)
			{
				throw spk::Exception("Voxel biome stratum has no positive material weight");
			}
			double sample;
			switch (stratum.distribution)
			{
			case Stratum::Distribution::Weighted:
				sample = _weightedSample(position, stratum.seed);
				break;
			case Stratum::Distribution::Perlin:
				sample = stratum.materialNoise.sample2D(position.x, position.y);
				break;
			case Stratum::Distribution::Custom:
				if (!stratum.customDistribution)
				{
					throw spk::Exception("Voxel biome stratum has no custom distribution");
				}
				sample = stratum.customDistribution(position);
				break;
			default:
				throw spk::Exception("Unknown voxel biome material distribution");
			}
			if (!std::isfinite(sample) || sample < 0.0 || sample > 1.0)
			{
				throw spk::Exception("Voxel biome material sample must be in [0, 1]");
			}
			const double target = sample * total;
			double cumulative = 0.0;
			std::uint32_t id = 0;
			for (const auto &material : stratum.materials)
			{
				if (material.weight <= 0.0f)
				{
					continue;
				}
				id = material.voxelID;
				cumulative += material.weight;
				if (target < cumulative)
				{
					break;
				}
			}
			if (stratum.thicknessMode == Stratum::ThicknessMode::Infinite)
			{
				return {std::nullopt, id};
			}
			if (stratum.thicknessMode != Stratum::ThicknessMode::Finite ||
				stratum.minimumThickness == 0 || stratum.minimumThickness > stratum.maximumThickness)
			{
				throw spk::Exception("Invalid voxel biome stratum thickness");
			}
			const double range = static_cast<double>(stratum.maximumThickness) - stratum.minimumThickness + 1.0;
			const double noise = stratum.minimumThickness == stratum.maximumThickness ? 0.0 : stratum.thicknessNoise.sample2D(position.x, position.y);
			const auto offset = std::min(range - 1.0, std::floor(noise * range));
			return {stratum.minimumThickness + static_cast<std::uint32_t>(offset), id};
		}

	public:
		[[nodiscard]] StratumContainer &strata() noexcept
		{
			return _strata;
		}
		[[nodiscard]] const StratumContainer &strata() const noexcept
		{
			return _strata;
		}

		template <typename TCallback>
		void sampleStrata(const spk::Vector2 &position, TCallback &&callback) const
		{
			if (!std::isfinite(position.x) || !std::isfinite(position.y))
			{
				throw spk::Exception("Voxel biome stratum position must be finite");
			}
			for (std::size_t index = 0; index < _strata.size(); ++index)
			{
				const auto &stratum = _strata[index];
				if (stratum.thicknessMode == Stratum::ThicknessMode::Infinite && index + 1 != _strata.size())
				{
					throw spk::Exception("Only the final voxel biome stratum may be infinite");
				}
				callback(_sample(stratum, position));
			}
		}
	};
}
