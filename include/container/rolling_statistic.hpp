#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <vector>

#include "cached_data.hpp"
#include "exception.hpp"

namespace spk
{
	template <typename TType>
		requires requires(TType value, std::size_t count) {
			{ value + value } -> std::convertible_to<TType>;
			{ value - value } -> std::convertible_to<TType>;
			{ value / count } -> std::convertible_to<TType>;
			{ value < value } -> std::convertible_to<bool>;
		}
	class RollingStatistic
	{
	public:
		using Count = std::size_t;

	private:
		std::vector<TType> _values;
		TType _total{};
		Count _cursor = 0;
		Count _nbSample = 0;

		CachedData<TType> _min{
			[this]() {
				return *std::min_element(
					_values.begin(),
					_values.begin() + _nbSample);
			}};

		CachedData<TType> _max{
			[this]() {
				return *std::max_element(
					_values.begin(),
					_values.begin() + _nbSample);
			}};

		void _throwIfEmpty() const
		{
			if (_nbSample == 0)
			{
				throw spk::Exception("RollingStatistic contains no sample");
			}
		}

	public:
		explicit RollingStatistic(Count capacity) :
			_values(capacity)
		{
			if (capacity == 0)
			{
				throw spk::Exception(
					"RollingStatistic capacity must be greater than zero");
			}
		}

		RollingStatistic(const RollingStatistic &) = delete;
		RollingStatistic &operator=(const RollingStatistic &) = delete;
		RollingStatistic(RollingStatistic &&) = delete;
		RollingStatistic &operator=(RollingStatistic &&) = delete;

		void insert(const TType &value)
		{
			if (_nbSample < _values.size())
			{
				_values[_cursor] = value;
				_total = _total + value;
				++_nbSample;
			}
			else
			{
				_total = _total - _values[_cursor];
				_values[_cursor] = value;
				_total = _total + value;
			}

			_cursor = (_cursor + 1) % _values.size();

			_min.invalidate();
			_max.invalidate();
		}

		void reset()
		{
			_total = TType{};
			_cursor = 0;
			_nbSample = 0;

			_min.invalidate();
			_max.invalidate();
		}

		[[nodiscard]] TType average() const
		{
			_throwIfEmpty();
			return _total / static_cast<TType>(_nbSample);
		}

		[[nodiscard]] const TType &min() const
		{
			_throwIfEmpty();
			return _min.get();
		}

		[[nodiscard]] const TType &max() const
		{
			_throwIfEmpty();
			return _max.get();
		}

		[[nodiscard]] Count nbSample() const
		{
			return _nbSample;
		}

		[[nodiscard]] Count capacity() const
		{
			return _values.size();
		}

		[[nodiscard]] bool full() const
		{
			return _nbSample == _values.size();
		}
	};
}