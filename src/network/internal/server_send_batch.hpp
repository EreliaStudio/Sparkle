#pragma once

#include "network/internal/transfer_completion.hpp"
#include "network/types.hpp"

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
