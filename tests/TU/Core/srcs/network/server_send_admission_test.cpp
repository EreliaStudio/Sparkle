#include "network/client.hpp"
#include "network/internal/server_handshake.hpp"
#include "network/internal/server_send_admission.hpp"
#include "network/internal/transfer_sender.hpp"
#include "network/server.hpp"
#include "network_test_utils.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <new>
#include <stdexcept>
#include <unordered_map>

namespace
{
	using namespace spk::NetworkInternal;

	// Admission customizes queue/write hooks; this session uses the real sender
	// and reservations while allowing a queue failure before the second commit.
	struct AdmissionSession
	{
		spk::Connection::ID id;
		ServerHandshake handshake;
		TransferSender transfers;
		std::size_t reservedOutgoingBytes = 0;
		std::unordered_map<spk::Transfer::ID, OutgoingReservation> outgoingReservations;

		explicit AdmissionSession(spk::Connection::ID id) :
			id(id)
		{
			(void)handshake.accept(*Handshake::encode(Handshake::Kind::Hello));
			(void)handshake.written(Handshake::Kind::Accepted);
		}
	};

	void verifyCommitFailure(bool nestedCause)
	{
		spk::Server server;
		spk::Client first;
		spk::Client second;
		server.start(0);
		first.connect("127.0.0.1", server.port());
		second.connect("127.0.0.1", server.port());
		ASSERT_TRUE(NetworkTestUtils::waitConnected(first));
		ASSERT_TRUE(NetworkTestUtils::waitConnected(second));
		const auto firstConnection = server.connection(first.connection().id());
		const auto secondConnection = server.connection(second.connection().id());
		ASSERT_TRUE(NetworkTestUtils::waitConnected(firstConnection));
		ASSERT_TRUE(NetworkTestUtils::waitConnected(secondConnection));
		ServerSendBatch batch(spk::Message::Writer(280).build(), spk::Message::Priority::Normal, spk::Connection::OverflowPolicy::Reject);
		batch.recipients.push_back({firstConnection, spk::Transfer{}});
		batch.recipients.push_back({secondConnection, spk::Transfer{}});
		std::unordered_map<spk::Connection::ID, std::shared_ptr<AdmissionSession>> sessions;
		for (const auto &recipient : batch.recipients)
		{
			sessions.emplace(recipient.connection.id(), std::make_shared<AdmissionSession>(recipient.connection.id()));
		}
		CommandBudget budget(4096);
		const std::function<std::vector<spk::Connection>()> snapshot = [] {
			return std::vector<spk::Connection>{};
		};
		ServerSendAdmission<AdmissionSession> admission(batch, sessions, budget, snapshot);
		std::size_t queued = 0;
		std::size_t flushed = 0;
		try
		{
			admission.run([&](const auto &session, auto &recipient) {
				if (++queued == 2)
				{
					if (!nestedCause)
					{
						throw std::bad_alloc();
					}
					try
					{
						throw std::runtime_error("original queue failure");
					} catch (...)
					{
						throw spk::Exception("queue context", std::current_exception());
					}
				}
				const auto transfer = session->transfers.enqueue(batch.message, batch.priority, recipient.transfer);
				session->reservedOutgoingBytes += recipient.reservation.bytes();
				session->outgoingReservations.emplace(transfer.id(), std::move(recipient.reservation));
			},
						  [&flushed](const auto &) {
							  ++flushed;
						  },
						  [](auto) {
							  FAIL() << "Admission rollback must not disconnect a recipient.";
						  });
			FAIL() << "Queue failure must propagate.";
		} catch (const spk::Exception &error)
		{
			EXPECT_EQ(error.message(), "Unable to commit atomic network send admission.");
			ASSERT_NE(error.cause(), nullptr);
			if (nestedCause)
			{
				EXPECT_NE(std::string(error.what()).find("queue context"), std::string::npos);
				EXPECT_NE(std::string(error.what()).find("original queue failure"), std::string::npos);
			}
			else
			{
				EXPECT_THROW(std::rethrow_exception(error.cause()), std::bad_alloc);
			}
		}
		EXPECT_EQ(flushed, 0u);
		EXPECT_EQ(budget.used(), 0u);
		EXPECT_EQ(batch.recipients.front().transfer.status(), spk::Transfer::Status::Rejected);
		for (const auto &[id, session] : sessions)
		{
			(void)id;
			EXPECT_EQ(session->reservedOutgoingBytes, 0u);
			EXPECT_TRUE(session->outgoingReservations.empty());
			EXPECT_EQ(session->transfers.pendingTransfers(), 0u);
		}
	}
}

TEST(ServerSendAdmission, RollbackPreservesAllocatorFailure)
{
	verifyCommitFailure(false);
}

TEST(ServerSendAdmission, RollbackPreservesNestedQueueContext)
{
	verifyCommitFailure(true);
}
