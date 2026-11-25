#pragma once

#include <cstddef>
#include <limits>
#include <vector>

#include "container/capacity_pool.hpp"

namespace spk
{
	template <typename TElement>
	class Grid2D : public LifeCycle
	{
	public:
		using Lease = typename CapacityPool<Grid2D>::Lease;
		using Reference = typename std::vector<TElement>::reference;
		using ConstReference = typename std::vector<TElement>::const_reference;

		struct Layout
		{
			std::size_t width;
			std::size_t height;

			[[nodiscard]] std::size_t count() const
			{
				if (width == 0 || height == 0 || width > std::numeric_limits<std::size_t>::max() / height)
				{
					throw spk::Exception("Invalid Grid2D dimensions");
				}
				return width * height;
			}
		};

	private:
		Layout _layout{0, 0};
		std::vector<TElement> _elements;
		ReleaseContract _releaseContract;

		explicit Grid2D(std::size_t capacity) :
			_releaseContract(subscribeToRelease([this]() {
				_elements.clear();
				_layout = {0, 0};
			}))
		{
			_elements.reserve(capacity);
		}

		[[nodiscard]] std::size_t _index(std::size_t x, std::size_t y) const
		{
			if (x >= _layout.width || y >= _layout.height)
			{
				throw spk::Exception("Grid2D index is outside its domain");
			}
			return y * _layout.width + x;
		}

	public:
		Grid2D(const Grid2D &) = delete;
		Grid2D &operator=(const Grid2D &) = delete;

		~Grid2D() override
		{
			terminateLifeCycle();
		}

		[[nodiscard]] static Lease obtain(std::size_t width, std::size_t height)
		{
			const Layout layout{width, height};
			const auto count = layout.count();
			if (CapacityPool<Grid2D>::capacityFor(count) > std::vector<TElement>().max_size())
			{
				throw spk::Exception("Grid2D exceeds storage limits");
			}
			static CapacityPool<Grid2D> pool([](std::size_t capacity) {
				return new Grid2D(capacity);
			});
			return pool.obtain(count, [&](Grid2D &grid) {
				grid._elements.resize(count);
				grid._layout = layout;
			});
		}

		[[nodiscard]] Layout layout() const noexcept
		{
			return _layout;
		}

		[[nodiscard]] std::size_t capacity() const noexcept
		{
			return _elements.capacity();
		}

		[[nodiscard]] Reference at(std::size_t x, std::size_t y)
		{
			return _elements[_index(x, y)];
		}

		[[nodiscard]] ConstReference at(std::size_t x, std::size_t y) const
		{
			return _elements[_index(x, y)];
		}
	};
}
