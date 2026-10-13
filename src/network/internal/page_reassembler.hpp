#pragma once

#include "network/internal/page.hpp"
#include "network/internal/page_failure_limiter.hpp"
#include "network/internal/page_reassembly_error.hpp"
#include "network/internal/reassembly_budget.hpp"
#include "network/internal/retired_transfer_ids.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace spk::NetworkInternal
{
	class PageReassembler final
	{
	public:
		using Clock = std::chrono::steady_clock;

	private:
		struct Transfer final
		{
			Message::Writer writer;
			std::map<std::uint64_t, std::size_t> portions;
			std::size_t receivedBytes = 0;
			std::size_t chargedBytes = 0;
			Clock::time_point updated;

			explicit Transfer(const PageFragment &first, Clock::time_point now);
		};

		std::unordered_map<std::uint64_t, Transfer> _transfers;
		RetiredTransferIDs _retiredTransfers;
		PageFailureLimiter _failureLimiter;
		std::size_t _reservedBytes = 0;
		std::size_t _chargedBytes = 0;
		std::shared_ptr<ReassemblyBudget> _sharedBudget;
		std::size_t _maxMessageBytes;
		std::size_t _maxIncompleteBytes;
		std::chrono::milliseconds _timeout;

		void _validateNew(const PageFragment &page) const;
		void _insert(Transfer &transfer, const PageFragment &page, Clock::time_point now);
		void _reserve(std::size_t bytes, std::uint64_t id);
		void _release(std::size_t bytes) noexcept;
		[[nodiscard]] bool _duplicate(const Transfer &transfer, const PageFragment &page) const;

	public:
		explicit PageReassembler(
			std::size_t maxMessageBytes = 128u * 1024u * 1024u,
			std::size_t maxIncompleteBytes = 512u * 1024u * 1024u,
			std::chrono::milliseconds timeout = std::chrono::seconds(60),
			std::shared_ptr<ReassemblyBudget> sharedBudget = {});
		~PageReassembler();
		PageReassembler(const PageReassembler &) = delete;
		PageReassembler &operator=(const PageReassembler &) = delete;

		[[nodiscard]] std::optional<Message> accept(const PageFragment &page, Clock::time_point now = Clock::now());
		std::vector<std::uint64_t> expire(Clock::time_point now = Clock::now());
		void clear() noexcept;
		void setInactivityTimeout(std::chrono::milliseconds timeout);
		void setLimits(std::size_t maximumMessageBytes, std::size_t maximumIncompleteBytes);
		[[nodiscard]] std::size_t reservedBytes() const noexcept;
		[[nodiscard]] std::size_t pendingTransfers() const noexcept;
		[[nodiscard]] bool failureLimitReached() const;
		void configureFailureLimits(PageFailureLimiter::Config config);
		void successfulMessage();
	};
}
