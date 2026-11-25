#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
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

		using Layout = typename spk::Grid2D<TElement>::Layout;

	private:
		Configuration _configuration{};
		spk::Vector2 _origin{};
		Layout _layout{0, 0};
		typename spk::Grid2D<TElement>::Lease _elements;
		ReleaseContract _releaseContract;

		static std::size_t _axisCount(unsigned int size, float scale)
		{
			const double quotient = static_cast<double>(size) / scale;
			const double rounded = std::round(quotient);
			const double tolerance = std::min(1.0e-5, 4.0 * std::numeric_limits<float>::epsilon() * std::abs(quotient));
			if (!std::isfinite(quotient) || std::abs(quotient - rounded) > tolerance ||
				rounded >= static_cast<double>(std::numeric_limits<std::size_t>::max()))
			{
				throw spk::Exception("Surface grid extent must be divisible by its scale");
			}
			return static_cast<std::size_t>(rounded) + 1;
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
			const auto layout = layoutFor(configuration);
			static spk::Pool<TGrid> pool(factory);
			return pool.obtain([&](TGrid &grid) {
				grid._configure(configuration, origin, layout);
				configure(grid);
			});
		}

		void _configure(const Configuration &configuration, const spk::Vector2 &origin, Layout layout)
		{
			if (!std::isfinite(origin.x) || !std::isfinite(origin.y))
			{
				throw spk::Exception("Surface grid origin must be finite");
			}
			_elements = spk::Grid2D<TElement>::obtain(layout.width, layout.height);
			_configuration = configuration;
			_origin = origin;
			_layout = layout;
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

		[[nodiscard]] static Layout layoutFor(const Configuration &configuration)
		{
			if (!std::isfinite(configuration.scale) || configuration.scale <= 0.0f)
			{
				throw spk::Exception("Surface grid scale must be finite and positive");
			}
			const Layout layout{_axisCount(configuration.size.x, configuration.scale), _axisCount(configuration.size.y, configuration.scale)};
			if (layout.count() > std::vector<TElement>().max_size())
			{
				throw spk::Exception("Surface grid exceeds storage limits");
			}
			return layout;
		}

		[[nodiscard]] const Configuration &configuration() const noexcept
		{
			return _configuration;
		}
		[[nodiscard]] const spk::Vector2 &worldOrigin() const noexcept
		{
			return _origin;
		}
		[[nodiscard]] Layout layout() const noexcept
		{
			return _layout;
		}
		[[nodiscard]] std::size_t capacity() const noexcept
		{
			return _elements ? _elements->capacity() : 0;
		}

		[[nodiscard]] typename spk::Grid2D<TElement>::Reference sample(std::size_t x, std::size_t z)
		{
			return _elements->at(x, z);
		}
		[[nodiscard]] typename spk::Grid2D<TElement>::ConstReference sample(std::size_t x, std::size_t z) const
		{
			if (x >= _layout.width || z >= _layout.height)
			{
				throw spk::Exception("Surface grid sample index is outside its domain");
			}
			return _elements->at(x, z);
		}
		[[nodiscard]] spk::Vector2 localPosition(std::size_t x, std::size_t z) const
		{
			if (x >= _layout.width || z >= _layout.height)
			{
				throw spk::Exception("Surface grid sample index is outside its domain");
			}
			return {
				x == _layout.width - 1 ? static_cast<float>(_configuration.size.x) : static_cast<float>(x * static_cast<double>(_configuration.scale)),
				z == _layout.height - 1 ? static_cast<float>(_configuration.size.y) : static_cast<float>(z * static_cast<double>(_configuration.scale))};
		}
		[[nodiscard]] typename spk::Grid2D<TElement>::Reference at(const spk::Vector2 &position)
		{
			return sample(static_cast<std::size_t>(_sampleAxis(position.x, _configuration.size.x, _layout.width)), static_cast<std::size_t>(_sampleAxis(position.y, _configuration.size.y, _layout.height)));
		}
		[[nodiscard]] typename spk::Grid2D<TElement>::ConstReference at(const spk::Vector2 &position) const
		{
			return sample(static_cast<std::size_t>(_sampleAxis(position.x, _configuration.size.x, _layout.width)), static_cast<std::size_t>(_sampleAxis(position.y, _configuration.size.y, _layout.height)));
		}
	};
}
