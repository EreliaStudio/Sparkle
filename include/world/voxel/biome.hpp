#pragma once

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <type_traits>
#include <variant>
#include <vector>

#include "math/perlin.hpp"
#include "voxel/volume.hpp"
#include "world/surface_biome.hpp"

namespace spk::World::Voxel
{
	class Biome : public World::SurfaceBiome
	{
	public:
		template <std::size_t TCapacity = 4>
		class GenerationData;

		template <typename TChunk, std::size_t TCapacity = 4>
		class Generator;

		struct Stratum
		{
			struct Material
			{
				std::uint32_t voxelID;
				float weight;
			};

			struct Thickness
			{
				struct Fixed
				{
					std::uint32_t value = 1;
				};

				struct Variable
				{
					std::uint32_t minimum = 1;
					std::uint32_t maximum = 1;
					spk::Perlin noise{};
				};

				struct Infinite
				{
				};

				using Configuration = std::variant<Fixed, Variable, Infinite>;
			};

			struct Distribution
			{
				struct Weighted
				{
					std::uint32_t seed = 0;
				};

				struct Perlin
				{
					spk::Perlin noise{};
				};

				struct Custom
				{
					std::function<float(const spk::Vector2 &)> function;
				};

				using Configuration = std::variant<Weighted, Perlin, Custom>;
			};

			Thickness::Configuration thickness = Thickness::Fixed{1};
			Distribution::Configuration distribution = Distribution::Weighted{0};
			std::vector<Material> materials;
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
				if (material.voxelID == 0 || material.voxelID > spk::Voxel::Volume::PackedCell::MaxDefinitionId ||
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
			const double sample = std::visit(
				[&](const auto &configuration) -> double {
					using Configuration = std::decay_t<decltype(configuration)>;
					if constexpr (std::is_same_v<Configuration, Stratum::Distribution::Weighted>)
					{
						return _weightedSample(position, configuration.seed);
					}
					else if constexpr (std::is_same_v<Configuration, Stratum::Distribution::Perlin>)
					{
						return configuration.noise.sample2D(position.x, position.y);
					}
					else
					{
						if (!configuration.function)
						{
							throw spk::Exception("Voxel biome stratum has no custom distribution");
						}
						return configuration.function(position);
					}
				},
				stratum.distribution);
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
			const auto thickness = std::visit(
				[&](const auto &configuration) -> std::optional<std::uint32_t> {
					using Configuration = std::decay_t<decltype(configuration)>;
					if constexpr (std::is_same_v<Configuration, Stratum::Thickness::Infinite>)
					{
						return std::nullopt;
					}
					else if constexpr (std::is_same_v<Configuration, Stratum::Thickness::Fixed>)
					{
						if (configuration.value == 0)
						{
							throw spk::Exception("Invalid voxel biome stratum thickness");
						}
						return configuration.value;
					}
					else
					{
						if (configuration.minimum == 0 || configuration.minimum > configuration.maximum)
						{
							throw spk::Exception("Invalid voxel biome stratum thickness");
						}
						const double range = static_cast<double>(configuration.maximum) - configuration.minimum + 1.0;
						const double noise = configuration.minimum == configuration.maximum ? 0.0 : configuration.noise.sample2D(position.x, position.y);
						const auto offset = std::min(range - 1.0, std::floor(noise * range));
						return configuration.minimum + static_cast<std::uint32_t>(offset);
					}
				},
				stratum.thickness);
			return {thickness, id};
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
				if (std::holds_alternative<Stratum::Thickness::Infinite>(stratum.thickness) && index + 1 != _strata.size())
				{
					throw spk::Exception("Only the final voxel biome stratum may be infinite");
				}
				callback(_sample(stratum, position));
			}
		}
	};
}
