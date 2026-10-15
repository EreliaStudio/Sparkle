#pragma once

#include "network/internal/outgoing_reservation.hpp"
#include "network/internal/transfer_sender.hpp"
#include "network/internal/transmission_frame.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace spk::NetworkInternal
{
	class ClientTransmission final
	{
	private:
		std::mutex _mutex;
		std::condition_variable _available;
		TransferSender _transfers;
		std::deque<TransmissionFrame> _responses;
		CommandBudget _budget{64u * 1024u * 1024u};
		std::unordered_map<spk::Transfer::ID, OutgoingReservation> _reservations;
		bool _accepting = false;

	public:
		void start();
		void stop() noexcept;
		Transfer send(const Message &message, Message::Priority priority, std::size_t queuedCapacity, std::size_t maxPageBytes);
		void queueResponse(const TransferResponse &response);
		void acknowledge(const TransferResponse &response);
		[[nodiscard]] std::optional<TransmissionFrame> nextFrame(const std::atomic_bool &connected, std::size_t maxPageBytes);
		void frameWritten(spk::Transfer::ID finalPage);
		void expire();
		void setMaxMessageBytes(std::size_t bytes);
		void setAcknowledgmentTimeout(std::chrono::milliseconds timeout);
		void wake() noexcept;
		void clear() noexcept;
	};
}
