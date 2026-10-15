#include <gtest/gtest.h>

#include "exception.hpp"
#include "network/server.hpp"

#include <chrono>

TEST(ServerTest, StartsOnRequestedEphemeralPortAndStops)
{
	spk::Server server;

	server.start(0);

	EXPECT_TRUE(server.isRunning());
	EXPECT_NE(server.port(), 0u);

	server.stop();
	EXPECT_FALSE(server.isRunning());
	EXPECT_EQ(server.port(), 0u);
}

TEST(ServerTest, StartingRunningServerThrowsWithoutReplacingListener)
{
	spk::Server server;
	server.start(0);
	const auto originalPort = server.port();
	EXPECT_THROW(server.start(0), spk::Exception);
	EXPECT_TRUE(server.isRunning());
	EXPECT_EQ(server.port(), originalPort);
	server.stop();
}

TEST(ServerTest, ExplicitRestartReplacesRunningServer)
{
	spk::Server server;
	server.start(0);
	EXPECT_NE(server.port(), 0u);
	server.start(0, spk::Server::StartBehaviour::Restart);
	EXPECT_TRUE(server.isRunning());
	EXPECT_NE(server.port(), 0u);
	server.stop();
}

TEST(ServerTest, RejectsUnsupportedStartBehaviourWithoutAffectingServer)
{
	spk::Server server;
	server.start(0);
	const auto originalPort = server.port();
	EXPECT_THROW(server.start(0, static_cast<spk::Server::StartBehaviour>(99)), spk::Exception);
	EXPECT_EQ(server.port(), originalPort);
	EXPECT_TRUE(server.isRunning());
	server.stop();
}

TEST(ServerTest, ExplicitPoliciesRejectUnknownValues)
{
	spk::Server server;
	const auto handshake = static_cast<spk::Server::HandshakeTimeoutBehaviour>(99);
	const auto outgoing = static_cast<spk::Server::OutgoingBudgetBehaviour>(99);
	const auto incoming = static_cast<spk::Server::IncomingBudgetBehaviour>(99);
	const auto shared = static_cast<spk::Server::SharedIncomingBudgetBehaviour>(99);
	const auto pageSize = static_cast<spk::Server::PageSizeBehaviour>(99);
	const auto send = static_cast<spk::Server::SendBehaviour>(99);
	const auto emptyAudience = static_cast<spk::Server::EmptyAudienceBehaviour>(99);
	const auto dispatch = static_cast<spk::Server::MessageDispatchBehaviour>(99);
	EXPECT_THROW(server.setHandshakeTimeout(std::chrono::milliseconds(250), handshake), spk::Exception);
	EXPECT_THROW(server.setGlobalMaxQueuedBytes(4096, outgoing), spk::Exception);
	EXPECT_THROW(server.setMaxIncomingBytes(4096, incoming), spk::Exception);
	EXPECT_THROW(server.setGlobalMaxIncomingBytes(4096, shared), spk::Exception);
	EXPECT_THROW(server.setMaxPageBytes(4096, pageSize), spk::Exception);
	EXPECT_THROW(server.treatMessages(dispatch), spk::Exception);
	server.start(0);
	EXPECT_THROW(server.sendToAll(spk::Message::Writer(1).build(), spk::Message::Priority::Normal, spk::Connection::OverflowPolicy::Reject, send), spk::Exception);
	EXPECT_THROW(server.sendToAll(spk::Message::Writer(1).build(), spk::Message::Priority::Normal, spk::Connection::OverflowPolicy::Reject, spk::Server::SendBehaviour::SynchronousAdmission, emptyAudience), spk::Exception);
	EXPECT_TRUE(server.sendToAll(spk::Message::Writer(1).build(), spk::Message::Priority::Normal, spk::Connection::OverflowPolicy::Reject, spk::Server::SendBehaviour::SynchronousAdmission, spk::Server::EmptyAudienceBehaviour::ReturnEmpty).empty());
	server.stop();
}

TEST(ServerTest, BroadcastWithoutRecipientsReturnsEmpty)
{
	spk::Server server;
	server.start(0);
	EXPECT_TRUE(server.sendToAll(spk::Message::Writer(1).build()).empty());
	EXPECT_THROW(server.sendTo(spk::ConnectionSet{}, spk::Message::Writer(1).build()), spk::Exception);
	server.stop();
}

TEST(ServerTest, SendingWhileStoppedThrows)
{
	spk::Server server;

	EXPECT_THROW(server.sendToAll(spk::Message::Writer(1).build()), spk::Exception);
}

TEST(ServerTest, RejectsInvalidConnectionHandle)
{
	spk::Server server;
	server.start(0);
	EXPECT_THROW(server.sendTo(spk::Connection(), spk::Message::Writer(1).build()), spk::Exception);
	EXPECT_THROW(server.sendTo(spk::Connection(), spk::Message::Writer(1).build(), spk::Message::Priority::Normal, spk::Connection::OverflowPolicy::SkipRecipient), spk::Exception);
	server.stop();
}

TEST(ServerTest, TransferTimeoutConfigurationRejectsNonPositiveValues)
{
	spk::Server server;
	EXPECT_THROW(server.setIncomingTransferTimeout(std::chrono::milliseconds(0)), spk::Exception);
	EXPECT_THROW(server.setAcknowledgmentTimeout(std::chrono::milliseconds(-1)), spk::Exception);
	EXPECT_NO_THROW(server.setIncomingTransferTimeout(std::chrono::milliseconds(250)));
	EXPECT_NO_THROW(server.setAcknowledgmentTimeout(std::chrono::milliseconds(250)));
	server.start(0);
	EXPECT_NO_THROW(server.setIncomingTransferTimeout(std::chrono::milliseconds(500)));
	EXPECT_NO_THROW(server.setAcknowledgmentTimeout(std::chrono::milliseconds(500)));
	server.stop();
}
