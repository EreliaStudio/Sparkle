#pragma once

#include "network/internal/command_budget.hpp"
#include "network/internal/transfer_completion.hpp"
#include "network/types.hpp"

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
			std::shared_ptr<CommandBudget::Lease> reservation;
		};

		Message message;
		Message::Priority priority;
		Connection::OverflowPolicy policy;
		std::vector<Recipient> recipients;

		ServerSendBatch(const Message &message, Message::Priority priority, Connection::OverflowPolicy policy) :
			message(message),
			priority(priority),
			policy(policy)
		{
		}

		void rejectAll(const std::vector<std::pair<Recipient *, Transfer::Reason>> &declined) noexcept
		{
			for (Recipient &recipient : recipients)
			{
				Transfer::Reason reason = Transfer::Reason::AtomicBatchRejected;
				for (const auto &[rejected, cause] : declined)
				{
					if (rejected == &recipient)
					{
						reason = cause;
						break;
					}
				}
				(void)TransferCompletion::complete(recipient.transfer, Transfer::Status::Rejected, reason);
			}
		}

		~ServerSendBatch() noexcept
		{
			for (Recipient &recipient : recipients)
			{
				if (!recipient.admitted)
				{
					(void)TransferCompletion::complete(recipient.transfer, Transfer::Status::Indeterminate, Transfer::Reason::TransportFailure);
				}
			}
		}
	};
}
