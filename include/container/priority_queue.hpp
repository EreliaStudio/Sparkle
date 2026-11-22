#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <utility>
#include <vector>

namespace spk
{
	// Ascending comparator order; equal priorities preserve insertion order.
	// Only const access is provided so clients cannot silently unsort the queue.
	template <typename T, typename Comparator = std::less<T>>
	class PriorityQueue
	{
	public:
		using const_iterator = typename std::vector<T>::const_iterator;

	private:
		std::vector<T> _elements;
		Comparator _comparator;

	public:
		explicit PriorityQueue(Comparator comparator = Comparator{}) :
			_comparator(std::move(comparator))
		{
		}

		void pushBack(T element)
		{
			_elements.push_back(std::move(element));
			sort();
		}

		void pushBack(const std::vector<T> &elements)
		{
			_elements.insert(_elements.end(), elements.begin(), elements.end());
			sort();
		}

		void pushBack(std::vector<T> &&elements)
		{
			_elements.insert(
				_elements.end(),
				std::make_move_iterator(elements.begin()),
				std::make_move_iterator(elements.end()));
			sort();
		}

		template <typename TOtherComparator>
		void pushBack(const PriorityQueue<T, TOtherComparator> &other)
		{
			_elements.insert(_elements.end(), other.begin(), other.end());
			sort();
		}

		void sort()
		{
			std::stable_sort(_elements.begin(), _elements.end(), _comparator);
		}

		void clear() noexcept
		{
			_elements.clear();
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return _elements.empty();
		}

		[[nodiscard]] std::size_t size() const noexcept
		{
			return _elements.size();
		}

		[[nodiscard]] const T &operator[](std::size_t index) const noexcept
		{
			return _elements[index];
		}

		[[nodiscard]] const_iterator begin() const noexcept
		{
			return _elements.begin();
		}

		[[nodiscard]] const_iterator end() const noexcept
		{
			return _elements.end();
		}
	};
}
