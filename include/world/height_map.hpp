#pragma once

#include <cmath>
#include <map>
#include <string>

#include "container/capacity_pool.hpp"
#include "math/perlin.hpp"
#include "world/surface_grid.hpp"

namespace spk::World
{
	class HeightMap
	{
	public:
		struct NoiseLayer
		{
			spk::Perlin perlin;
			float minimum = 0.0f;
			float maximum = 1.0f;
		};

		using NoiseLayerCollection = std::map<std::string, NoiseLayer>;

		class Grid : public SurfaceGrid<float>
		{
		public:
			using Lease = spk::Pool<Grid>::Lease;
			enum class Interpolation { Nearest, Bilinear };

		private:
			explicit Grid(std::size_t capacity) : SurfaceGrid(capacity) {}

		public:
			using SurfaceGrid::at;

			[[nodiscard]] static Lease obtain(const Configuration &configuration, const spk::Vector2 &origin)
			{
				const auto layout = layoutFor(configuration);
				static spk::CapacityPool<Grid> pool([](std::size_t capacity) { return new Grid(capacity); });
				return pool.obtain(layout.count(), [&](Grid &grid) { grid._configure(configuration, origin, layout); });
			}

			[[nodiscard]] float at(const spk::Vector2 &position, Interpolation interpolation) const
			{
				const auto dimensions = layout();
				const auto &config = configuration();
				const double x = _sampleAxis(position.x, config.size.x, dimensions.width);
				const double z = _sampleAxis(position.y, config.size.y, dimensions.height);
				const auto x0 = static_cast<std::size_t>(x);
				const auto z0 = static_cast<std::size_t>(z);
				const auto x1 = std::min(x0 + 1, dimensions.width - 1);
				const auto z1 = std::min(z0 + 1, dimensions.height - 1);
				switch (interpolation)
				{
				case Interpolation::Nearest:
					return sample(x - x0 > 0.5 ? x1 : x0, z - z0 > 0.5 ? z1 : z0);
				case Interpolation::Bilinear:
					return std::lerp(std::lerp(sample(x0, z0), sample(x1, z0), static_cast<float>(x - x0)),
						std::lerp(sample(x0, z1), sample(x1, z1), static_cast<float>(x - x0)), static_cast<float>(z - z0));
				}
				throw spk::Exception("Unknown height grid interpolation");
			}
		};

	private:
		float _baseHeight = 0.0f;
		NoiseLayerCollection _noiseLayers;

	public:
		void setBaseHeight(float value)
		{
			if (!std::isfinite(value))
			{
				throw spk::Exception("HeightMap base height must be finite");
			}
			_baseHeight = value;
		}
		[[nodiscard]] float baseHeight() const noexcept { return _baseHeight; }
		[[nodiscard]] NoiseLayerCollection &noiseLayers() noexcept { return _noiseLayers; }
		[[nodiscard]] const NoiseLayerCollection &noiseLayers() const noexcept { return _noiseLayers; }

		[[nodiscard]] float sample(const spk::Vector2 &worldPosition) const
		{
			double result = _baseHeight;
			for (const auto &[name, layer] : _noiseLayers)
			{
				(void)name;
				result += layer.perlin.sample2D(worldPosition.x, worldPosition.y, layer.minimum, layer.maximum);
			}
			const float height = static_cast<float>(result);
			if (!std::isfinite(height))
			{
				throw spk::Exception("HeightMap returned a non-finite height");
			}
			return height;
		}

		[[nodiscard]] Grid::Lease generate(const Grid::Configuration &configuration, const spk::Vector2 &origin) const
		{
			auto grid = Grid::obtain(configuration, origin);
			const auto layout = grid->layout();
			for (std::size_t z = 0; z < layout.height; ++z)
			{
				for (std::size_t x = 0; x < layout.width; ++x)
				{
					grid->sample(x, z) = sample(origin + grid->localPosition(x, z));
				}
			}
			return grid;
		}
	};
}
