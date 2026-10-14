#pragma once

#include <cstddef>
#include <cstdint>
#include <map>

namespace spk::NetworkInternal
{
	class RetiredTransferIDs final
	{
	public:
		inline static constexpr std::size_t MaximumSparseRanges = 4096;

	private:
		std::uint64_t _prefix = 0;
		std::map<std::uint64_t, std::uint64_t> _ranges;

		void _compact() noexcept;

	public:
		[[nodiscard]] bool contains(std::uint64_t id) const noexcept;

		void retire(std::uint64_t id);

		[[nodiscard]] std::size_t rangeCount() const noexcept;

		void clear() noexcept;
	};
}
