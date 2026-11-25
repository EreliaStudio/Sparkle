#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

#include "container/capacity_pool.hpp"
#include "math/vector2.hpp"

namespace spk
{
	template <typename TElement>
	class Grid2D : public LifeCycle
	{
	public:
		using Lease = typename CapacityPool<Grid2D>::Lease;
		using Reference = typename std::vector<TElement>::reference;
		using ConstReference = typename std::vector<TElement>::const_reference;

	private:
		template <bool TConst>
		class BasicIterator
		{
		private:
			using Grid = std::conditional_t<TConst, const Grid2D, Grid2D>;
			Grid *_grid = nullptr;
			std::size_t _index = 0;

			friend class Grid2D;

			BasicIterator(Grid *grid, std::size_t index) noexcept :
				_grid(grid),
				_index(index)
			{
			}

		public:
			using iterator_category = std::input_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = std::pair<const spk::Vector2UInt, TElement>;
			using reference = std::pair<const spk::Vector2UInt, std::conditional_t<TConst, ConstReference, Reference>>;
			using pointer = void;

			BasicIterator() = default;

			[[nodiscard]] reference operator*() const
			{
				const auto width = _grid->_size.x;
				return {{static_cast<std::uint32_t>(_index % width), static_cast<std::uint32_t>(_index / width)}, _grid->_elements[_index]};
			}

			BasicIterator &operator++() noexcept
			{
				++_index;
				return *this;
			}

			BasicIterator operator++(int) noexcept
			{
				auto previous = *this;
				++(*this);
				return previous;
			}

			[[nodiscard]] bool operator==(const BasicIterator &) const noexcept = default;
		};

	public:
		using iterator = BasicIterator<false>;
		using const_iterator = BasicIterator<true>;

	private:
		spk::Vector2UInt _size{};
		std::vector<TElement> _elements;
		ReleaseContract _releaseContract;

		explicit Grid2D(std::size_t capacity) :
			_releaseContract(subscribeToRelease([this]() {
				_elements.clear();
				_size = {};
			}))
		{
			_elements.reserve(capacity);
		}

		[[nodiscard]] std::size_t _index(const spk::Vector2UInt &position) const
		{
			if (position.x >= _size.x || position.y >= _size.y)
			{
				throw spk::Exception("Grid2D index is outside its domain");
			}
			return static_cast<std::size_t>(position.y) * _size.x + position.x;
		}

	public:
		Grid2D(const Grid2D &) = delete;
		Grid2D &operator=(const Grid2D &) = delete;

		~Grid2D() override
		{
			terminateLifeCycle();
		}

		[[nodiscard]] static Lease obtain(const spk::Vector2UInt &size)
		{
			if (size.x == 0 || size.y == 0 || static_cast<std::size_t>(size.x) > std::numeric_limits<std::size_t>::max() / size.y)
			{
				throw spk::Exception("Invalid Grid2D dimensions");
			}
			const auto count = static_cast<std::size_t>(size.x) * size.y;
			if (CapacityPool<Grid2D>::capacityFor(count) > std::vector<TElement>().max_size())
			{
				throw spk::Exception("Grid2D exceeds storage limits");
			}
			static CapacityPool<Grid2D> pool([](std::size_t capacity) {
				return new Grid2D(capacity);
			});
			return pool.obtain(count, [&](Grid2D &grid) {
				grid._elements.resize(count);
				grid._size = size;
			});
		}

		[[nodiscard]] const spk::Vector2UInt &size() const noexcept
		{
			return _size;
		}

		[[nodiscard]] std::size_t count() const noexcept
		{
			return _elements.size();
		}

		[[nodiscard]] std::size_t capacity() const noexcept
		{
			return _elements.capacity();
		}

		[[nodiscard]] Reference operator[](const spk::Vector2UInt &position)
		{
			return at(position);
		}

		[[nodiscard]] ConstReference operator[](const spk::Vector2UInt &position) const
		{
			return at(position);
		}

		[[nodiscard]] Reference at(const spk::Vector2UInt &position)
		{
			return _elements[_index(position)];
		}

		[[nodiscard]] ConstReference at(const spk::Vector2UInt &position) const
		{
			return _elements[_index(position)];
		}

		[[nodiscard]] iterator begin() noexcept
		{
			return iterator(this, 0);
		}

		[[nodiscard]] iterator end() noexcept
		{
			return iterator(this, count());
		}

		[[nodiscard]] const_iterator begin() const noexcept
		{
			return cbegin();
		}

		[[nodiscard]] const_iterator end() const noexcept
		{
			return cend();
		}

		[[nodiscard]] const_iterator cbegin() const noexcept
		{
			return const_iterator(this, 0);
		}

		[[nodiscard]] const_iterator cend() const noexcept
		{
			return const_iterator(this, count());
		}
	};
}
