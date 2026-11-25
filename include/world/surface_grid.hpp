#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "container/grid_2d.hpp"
#include "exception.hpp"
#include "math/vector2.hpp"

namespace spk::World
{
	template <typename TElement>
	class SurfaceGrid : public spk::LifeCycle
	{
	public:
		struct Configuration
		{
			spk::Vector2UInt size;
			float scale = 1.0f;
			[[nodiscard]] bool operator==(const Configuration &) const = default;
		};

	private:
		Configuration _configuration{};
		spk::Vector2 _origin{};
		typename spk::Grid2D<TElement>::Lease _elements;
		ReleaseContract _releaseContract;

		static std::uint32_t _axisCount(std::uint32_t size, float scale)
		{
			const double quotient = static_cast<double>(size) / scale;
			const double rounded = std::round(quotient);
			const double tolerance = std::min(1.0e-5, 4.0 * std::numeric_limits<float>::epsilon() * std::abs(quotient));
			if (!std::isfinite(quotient) || std::abs(quotient - rounded) > tolerance ||
				rounded >= static_cast<double>(std::numeric_limits<std::uint32_t>::max()))
			{
				throw spk::Exception("Surface grid extent must be divisible by its scale");
			}
			return static_cast<std::uint32_t>(rounded) + 1;
		}

	protected:
		SurfaceGrid() :
			_releaseContract(subscribeToRelease([this]() {
				_elements = {};
			}))
		{
		}

		~SurfaceGrid() override
		{
			terminateLifeCycle();
		}

		template <typename TGrid, typename TFactory, typename TConfigure>
		[[nodiscard]] static typename spk::Pool<TGrid>::Lease _obtain(const Configuration &configuration, const spk::Vector2 &origin, TFactory factory, TConfigure configure)
		{
			const auto dimensions = sampleSizeFor(configuration);
			static spk::Pool<TGrid> pool(factory);
			return pool.obtain([&](TGrid &grid) {
				grid._configure(configuration, origin, dimensions);
				configure(grid);
			});
		}

		void _configure(const Configuration &configuration, const spk::Vector2 &origin, const spk::Vector2UInt &dimensions)
		{
			if (!std::isfinite(origin.x) || !std::isfinite(origin.y))
			{
				throw spk::Exception("Surface grid origin must be finite");
			}
			_elements = spk::Grid2D<TElement>::obtain(dimensions);
			_configuration = configuration;
			_origin = origin;
		}

		[[nodiscard]] double _sampleAxis(float position, unsigned int extent, std::size_t count) const
		{
			if (count == 0 || !std::isfinite(position) || position < 0.0f ||
				static_cast<double>(position) > static_cast<double>(extent))
			{
				throw spk::Exception("Surface grid position is outside its domain");
			}
			if (static_cast<double>(position) == static_cast<double>(extent))
			{
				return static_cast<double>(count - 1);
			}
			return std::min(static_cast<double>(position) / _configuration.scale, static_cast<double>(count - 1));
		}

	public:
		SurfaceGrid(const SurfaceGrid &) = delete;
		SurfaceGrid &operator=(const SurfaceGrid &) = delete;

		[[nodiscard]] static spk::Vector2UInt sampleSizeFor(const Configuration &configuration)
		{
			if (!std::isfinite(configuration.scale) || configuration.scale <= 0.0f)
			{
				throw spk::Exception("Surface grid scale must be finite and positive");
			}
			const spk::Vector2UInt dimensions{_axisCount(configuration.size.x, configuration.scale), _axisCount(configuration.size.y, configuration.scale)};
			if (static_cast<std::size_t>(dimensions.x) > std::vector<TElement>().max_size() / dimensions.y)
			{
				throw spk::Exception("Surface grid exceeds storage limits");
			}
			return dimensions;
		}

		[[nodiscard]] const Configuration &configuration() const noexcept
		{
			return _configuration;
		}
		[[nodiscard]] const spk::Vector2 &worldOrigin() const noexcept
		{
			return _origin;
		}
		[[nodiscard]] spk::Vector2UInt sampleSize() const noexcept
		{
			return _elements ? _elements->size() : spk::Vector2UInt{};
		}
		[[nodiscard]] std::size_t count() const noexcept
		{
			return _elements ? _elements->count() : 0;
		}
		[[nodiscard]] std::size_t capacity() const noexcept
		{
			return _elements ? _elements->capacity() : 0;
		}

		[[nodiscard]] TElement &sample(const spk::Vector2UInt &position)
		{
			return _elements->at(position);
		}
		[[nodiscard]] const TElement &sample(const spk::Vector2UInt &position) const
		{
			return _elements->at(position);
		}
		[[nodiscard]] spk::Vector2 localPosition(const spk::Vector2UInt &position) const
		{
			const auto dimensions = sampleSize();
			if (position.x >= dimensions.x || position.y >= dimensions.y)
			{
				throw spk::Exception("Surface grid sample index is outside its domain");
			}
			return {
				position.x == dimensions.x - 1 ? static_cast<float>(_configuration.size.x) : static_cast<float>(position.x * static_cast<double>(_configuration.scale)),
				position.y == dimensions.y - 1 ? static_cast<float>(_configuration.size.y) : static_cast<float>(position.y * static_cast<double>(_configuration.scale))};
		}
		[[nodiscard]] TElement &at(const spk::Vector2 &position)
		{
			return sample({static_cast<std::uint32_t>(_sampleAxis(position.x, _configuration.size.x, sampleSize().x)), static_cast<std::uint32_t>(_sampleAxis(position.y, _configuration.size.y, sampleSize().y))});
		}
		[[nodiscard]] const TElement &at(const spk::Vector2 &position) const
		{
			return sample({static_cast<std::uint32_t>(_sampleAxis(position.x, _configuration.size.x, sampleSize().x)), static_cast<std::uint32_t>(_sampleAxis(position.y, _configuration.size.y, sampleSize().y))});
		}
	};
}
