#pragma once

#include <concepts>
#include <cstddef>
#include <map>

#include "exception.hpp"

namespace spk
{
	template <typename TType>
		requires requires(TType value, std::size_t index) {
			{ TType{} < value } -> std::convertible_to<bool>;
			{ value / value };
			{ value * index } -> std::convertible_to<TType>;
		}
	class Histogram
	{
	public:
		using Count = std::size_t;
		using Index = std::size_t;

		struct Bucket
		{
			TType lowerBound{};
			TType upperBound{};
			Count count = 0;
		};

	private:
		TType _step;
		std::map<Index, Bucket> _buckets;
		Count _nbSample = 0;

		[[nodiscard]] Index _bucketIndex(const TType &value) const
		{
			return static_cast<Index>(value / _step);
		}

		[[nodiscard]] Bucket _createBucket(Index index) const
		{
			return Bucket{
				.lowerBound = static_cast<TType>(_step * index),
				.upperBound = static_cast<TType>(_step * (index + 1)),
				.count = 0};
		}

	public:
		explicit Histogram(const TType &step) :
			_step(step)
		{
			if (!(TType{} < _step))
			{
				throw spk::Exception("Histogram step must be greater than zero");
			}
		}

		void insert(const TType &value)
		{
			if (value < TType{})
			{
				throw spk::Exception("Histogram values must be greater than or equal to zero");
			}

			const Index index = _bucketIndex(value);

			auto [it, inserted] = _buckets.try_emplace(
				index,
				_createBucket(index));

			++it->second.count;
			++_nbSample;
		}

		void reset()
		{
			for (auto &[index, bucket] : _buckets)
			{
				bucket.count = 0;
			}

			_nbSample = 0;
		}

		[[nodiscard]] const TType &step() const
		{
			return _step;
		}

		[[nodiscard]] const std::map<Index, Bucket> &buckets() const
		{
			return _buckets;
		}

		[[nodiscard]] Count nbSample() const
		{
			return _nbSample;
		}
	};
}
