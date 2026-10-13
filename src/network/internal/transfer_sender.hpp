#pragma once

#include "network/internal/acknowledgment_deadlines.hpp"
#include "network/internal/page_scheduler.hpp"
#include "network/internal/transfer_response.hpp"

namespace spk::NetworkInternal
{
	class TransferSender final
	{
	private:
		PageScheduler _pages;
		AcknowledgmentDeadlines _deadlines;

	public:
		[[nodiscard]] spk::Transfer enqueue(Message message, Message::Priority priority = Message::Priority::Normal, spk::Transfer transfer = {});
		[[nodiscard]] bool fail(spk::Transfer::ID id, spk::Transfer::Status status, spk::Transfer::Reason reason);
		void setMaxPendingBytes(std::size_t bytes) noexcept;
		void setMaxMessageBytes(std::size_t bytes);
		[[nodiscard]] std::optional<Page> nextPage(std::size_t maxPageBytes);
		void finalPageWritten(std::uint64_t id, AcknowledgmentDeadlines::Clock::time_point now = AcknowledgmentDeadlines::Clock::now());
		[[nodiscard]] bool acknowledge(const TransferResponse &response);
		void expire(AcknowledgmentDeadlines::Clock::time_point now = AcknowledgmentDeadlines::Clock::now());
		void setAcknowledgmentTimeout(std::chrono::milliseconds timeout);
		[[nodiscard]] bool isTracked(spk::Transfer::ID id) const noexcept;
		[[nodiscard]] std::size_t pendingTransfers() const noexcept;
		[[nodiscard]] std::size_t pendingBytes() const noexcept;
		void clear() noexcept;
	};
}
