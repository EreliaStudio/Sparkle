#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

#include "exception.hpp"
#include "math/perlin.hpp"
#include "voxel/volume.hpp"
#include "world/generator.hpp"

namespace spk::World::Voxel
{
	// A reusable heightfield pass for real voxel chunks. All noise is sampled in
	// absolute world coordinates, so generation is independent of chunk order.
	// This pass only writes solid cells; subsequent passes may add decorations.
	template <typename TChunk, typename TGenerationData>
	class PerlinTerrainPass final : public World::Generator<TChunk, TGenerationData>::Pass
	{
	public:
		struct NoiseLayer
		{
			spk::Perlin noise;
			float amplitude = 1.0f;
		};

		struct MaterialLayer
		{
			std::uint32_t voxelID;
			std::uint32_t thickness;
		};

		using SurfaceSelector = std::function<std::uint32_t(std::int64_t, std::int64_t, std::int32_t)>;

	private:
		std::vector<NoiseLayer> _noiseLayers;
		std::vector<MaterialLayer> _subsurface;
		SurfaceSelector _surfaceSelector;
		float _baseHeight = 0.0f;
		std::int32_t _minimumHeight = 0;
		std::int32_t _maximumHeight = 0;
		std::uint32_t _surfaceVoxel = 0;
		std::uint32_t _deepVoxel = 0;

		[[nodiscard]] std::uint32_t _materialAtDepth(
			std::int64_t depth,
			std::int64_t x,
			std::int64_t z,
			std::int32_t top) const
		{
			if (depth == 0)
			{
				return _surfaceSelector ? _surfaceSelector(x, z, top) : _surfaceVoxel;
			}
			for (const MaterialLayer &layer : _subsurface)
			{
				if (depth <= layer.thickness)
				{
					return layer.voxelID;
				}
				depth -= layer.thickness;
			}
			return _deepVoxel;
		}

	public:
		void addNoiseLayer(spk::Perlin noise, float amplitude)
		{
			if (!std::isfinite(amplitude))
			{
				throw spk::Exception("Perlin terrain noise amplitude must be finite");
			}
			_noiseLayers.push_back({std::move(noise), amplitude});
		}

		void clearNoiseLayers() noexcept
		{
			_noiseLayers.clear();
		}

		void setBaseHeight(float height)
		{
			if (!std::isfinite(height))
			{
				throw spk::Exception("Perlin terrain base height must be finite");
			}
			_baseHeight = height;
		}

		void setHeightRange(std::int32_t minimum, std::int32_t maximum)
		{
			if (minimum > maximum)
			{
				throw spk::Exception("Perlin terrain minimum height exceeds maximum height");
			}
			_minimumHeight = minimum;
			_maximumHeight = maximum;
		}

		void setSurfaceVoxel(std::uint32_t id) noexcept
		{
			_surfaceVoxel = id;
		}

		void setSubsurfaceLayers(std::vector<MaterialLayer> layers)
		{
			for (const MaterialLayer &layer : layers)
			{
				if (layer.thickness == 0)
				{
					throw spk::Exception("Perlin terrain material layers must have positive thickness");
				}
			}
			_subsurface = std::move(layers);
		}

		void setDeepVoxel(std::uint32_t id) noexcept
		{
			_deepVoxel = id;
		}

		void setSurfaceSelector(SurfaceSelector selector)
		{
			_surfaceSelector = std::move(selector);
		}

		[[nodiscard]] std::int32_t computeHeight(std::int64_t worldX, std::int64_t worldZ) const
		{
			double elevation = _baseHeight;
			for (const NoiseLayer &layer : _noiseLayers)
			{
				elevation += static_cast<double>(layer.amplitude) *
							 static_cast<double>(layer.noise.sample2D(
								 static_cast<float>(worldX), static_cast<float>(worldZ), -1.0f, 1.0f));
			}
			const double clamped = std::clamp(
				elevation, static_cast<double>(_minimumHeight), static_cast<double>(_maximumHeight));
			return static_cast<std::int32_t>(std::floor(clamped));
		}

		void apply(const World::ChunkCoordinate &coordinate, TGenerationData &data) override
		{
			auto &builder = data.chunk(coordinate).builder;
			const World::ChunkWorldCoordinate origin = TChunk::chunkCoordinateToWorld(coordinate);
			for (std::int32_t z = 0; z < TChunk::Size.z; ++z)
			{
				for (std::int32_t x = 0; x < TChunk::Size.x; ++x)
				{
					const auto gx = origin.x + x;
					const auto gz = origin.z + z;
					const std::int32_t top = computeHeight(gx, gz);
					for (std::int32_t y = 0; y < TChunk::Size.y; ++y)
					{
						const std::int64_t gy = origin.y + y;
						if (gy > top)
						{
							break;
						}
						const std::uint32_t id = _materialAtDepth(
							static_cast<std::int64_t>(top) - gy, gx, gz, top);
						if (id != 0)
						{
							builder.set(
								{static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), static_cast<std::uint32_t>(z)},
								id);
						}
					}
				}
			}
		}
	};
}
