#pragma once

#include "exception.hpp"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <utility>

namespace spk::NetworkInternal
{
	class RetiredTransferIDs final
	{
	public:
		inline static constexpr std::size_t MaximumSparseRanges = 4096;

	private:
		std::uint64_t _prefix = 0;
		std::map<std::uint64_t, std::uint64_t> _ranges;

		void _compact() noexcept
		{
			while (!_ranges.empty())
			{
				if (_prefix == std::numeric_limits<std::uint64_t>::max() || _ranges.begin()->first != _prefix + 1)
				{
					break;
				}
				_prefix = _ranges.begin()->second;
				_ranges.erase(_ranges.begin());
			}
		}

	public:
		[[nodiscard]] bool contains(std::uint64_t id) const noexcept
		{
			if (id == 0)
			{
				return false;
			}
			if (id <= _prefix)
			{
				return true;
			}
			const auto next = _ranges.upper_bound(id);
			return next != _ranges.begin() && std::prev(next)->second >= id;
		}

		void retire(std::uint64_t id)
		{
			if (id == 0 || contains(id))
			{
				return;
			}
			if (_prefix != std::numeric_limits<std::uint64_t>::max() && id == _prefix + 1)
			{
				_prefix = id;
				_compact();
				return;
			}
			auto next = _ranges.upper_bound(id);
			if (next != _ranges.begin())
			{
				auto previous = std::prev(next);
				if (previous->second + 1 == id)
				{
					previous->second = id;
					if (next != _ranges.end() && id != std::numeric_limits<std::uint64_t>::max() &&
						next->first == id + 1)
					{
						previous->second = next->second;
						_ranges.erase(next);
					}
					_compact();
					return;
				}
			}
			if (next != _ranges.end() && id != std::numeric_limits<std::uint64_t>::max() &&
				next->first == id + 1)
			{
				auto node = _ranges.extract(next);
				node.key() = id;
				_ranges.insert(std::move(node));
			}
			else
			{
				if (_ranges.size() >= MaximumSparseRanges)
				{
					throw spk::Exception("Network transfer replay history exceeded its sparse range limit.");
				}
				_ranges.emplace(id, id);
			}
			_compact();
		}

		[[nodiscard]] std::size_t rangeCount() const noexcept
		{
			return _ranges.size();
		}

		void clear() noexcept
		{
			_prefix = 0;
			_ranges.clear();
		}
	};
}
