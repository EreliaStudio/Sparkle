#include "network/internal/server_send_batch.hpp"

#include "exception.hpp"
#include "network/internal/exception_reporting.hpp"
#include "network/internal/transfer_completion.hpp"

namespace spk::NetworkInternal
{
	void ServerSendBatch::acceptAdmission() noexcept
	{
		if (_admissionResolved)
		{
			return;
		}
		_admissionResolved = true;
		try
		{
			admission.set_value();
		} catch (...)
		{
			reportCurrentException("Network send admission promise resolution failed.");
		}
	}

	void ServerSendBatch::rejectAdmission(std::exception_ptr error) noexcept
	{
		if (_admissionResolved)
		{
			return;
		}
		_admissionResolved = true;
		try
		{
			admission.set_exception(error);
		} catch (...)
		{
			reportCurrentException("Network send admission promise resolution failed.");
		}
	}

	void ServerSendBatch::cancelAdmission() noexcept
	{
		try
		{
			rejectAdmission(std::make_exception_ptr(Exception("Network server stopped before send admission.")));
		} catch (...)
		{
			rejectAdmission(std::current_exception());
		}
	}

	ServerSendBatch::ServerSendBatch(const Message &message, Message::Priority priority, Connection::OverflowPolicy policy) :
		message(message),
		priority(priority),
		policy(policy)
	{
	}

	void ServerSendBatch::rejectAll(const std::vector<std::pair<Recipient *, Transfer::Reason>> &declined) noexcept
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

	ServerSendBatch::~ServerSendBatch() noexcept
	{
		if (!_admissionResolved)
		{
			cancelAdmission();
		}
		for (Recipient &recipient : recipients)
		{
			if (!recipient.admitted)
			{
				(void)TransferCompletion::complete(recipient.transfer, Transfer::Status::Indeterminate, Transfer::Reason::TransportFailure);
			}
		}
	}
}
