#pragma once

#include "network/internal/outgoing_reservation.hpp"
#include "network/transfer.hpp"
#include "network/types.hpp"

#include <exception>
#include <future>
#include <utility>
#include <vector>

namespace spk::NetworkInternal
{
	class ServerSendBatch final
	{
	public:
		struct Recipient
		{
			Connection connection;
			Transfer transfer;
			bool admitted = false;
			OutgoingReservation reservation;
		};

		Message message;
		Message::Priority priority;
		Connection::OverflowPolicy policy;
		std::vector<Recipient> recipients;
		bool allRecipients = false;
		std::promise<void> admission;

	private:
		bool _admissionResolved = false;

	public:
		ServerSendBatch(const Message &message, Message::Priority priority, Connection::OverflowPolicy policy);
		~ServerSendBatch() noexcept;

		void acceptAdmission() noexcept;

		void rejectAdmission(std::exception_ptr error) noexcept;

		void cancelAdmission() noexcept;

		void rejectAll(const std::vector<std::pair<Recipient *, Transfer::Reason>> &declined) noexcept;
	};
}
