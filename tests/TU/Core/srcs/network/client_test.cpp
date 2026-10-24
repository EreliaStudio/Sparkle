#include <gtest/gtest.h>

#include "exception.hpp"
#include "network/client.hpp"

#include <chrono>

TEST(ClientTest, StartsDisconnected)
{
	const spk::Client client;

	EXPECT_FALSE(client.isConnected());
}

TEST(ClientTest, SendingWhileDisconnectedThrows)
{
	spk::Client client;

	EXPECT_THROW(client.send(spk::Message::Writer(1).build()), spk::Exception);
}

TEST(ClientTest, TransferTimeoutConfigurationRejectsNonPositiveValues)
{
	spk::Client client;
	EXPECT_THROW(client.setHandshakeTimeout(std::chrono::milliseconds(0)), spk::Exception);
	EXPECT_THROW(client.setHandshakeTimeout(std::chrono::milliseconds(-1)), spk::Exception);
	EXPECT_NO_THROW(client.setHandshakeTimeout(std::chrono::milliseconds(250)));
	EXPECT_THROW(client.setIncomingTransferTimeout(std::chrono::milliseconds(0)), spk::Exception);
	EXPECT_THROW(client.setAcknowledgmentTimeout(std::chrono::milliseconds(-1)), spk::Exception);
	EXPECT_NO_THROW(client.setIncomingTransferTimeout(std::chrono::milliseconds(250)));
	EXPECT_NO_THROW(client.setAcknowledgmentTimeout(std::chrono::milliseconds(250)));
}

TEST(ClientTest, ExplicitPoliciesRejectUnknownValues)
{
	spk::Client client;
	const auto handshake = static_cast<spk::Client::HandshakeTimeoutBehaviour>(99);
	const auto incoming = static_cast<spk::Client::IncomingBudgetBehaviour>(99);
	const auto pageSize = static_cast<spk::Client::PageSizeBehaviour>(99);
	const auto send = static_cast<spk::Client::SendBehaviour>(99);
	const auto dispatch = static_cast<spk::Client::MessageDispatchBehaviour>(99);
	EXPECT_THROW(client.setHandshakeTimeout(std::chrono::milliseconds(250), handshake), spk::Exception);
	EXPECT_THROW(client.setMaxIncomingBytes(1024, incoming), spk::Exception);
	EXPECT_THROW(client.setMaxPageBytes(4096, pageSize), spk::Exception);
	EXPECT_THROW(client.send(spk::Message::Writer(1).build(), spk::Message::Priority::Normal, send), spk::Exception);
	EXPECT_THROW(client.treatMessages(dispatch), spk::Exception);
	EXPECT_NO_THROW(client.setHandshakeTimeout(std::chrono::milliseconds(250), spk::Client::HandshakeTimeoutBehaviour::FutureAttemptsOnly));
	EXPECT_NO_THROW(client.setMaxIncomingBytes(1024, spk::Client::IncomingBudgetBehaviour::AccountPayloadAndMetadataPreservingAccepted));
	EXPECT_NO_THROW(client.setMaxPageBytes(4096, spk::Client::PageSizeBehaviour::RespectNegotiatedLimit));
	EXPECT_NO_THROW(client.treatMessages(spk::Client::MessageDispatchBehaviour::CallingThread));
}

TEST(ClientTest, ConnectRejectsInvalidArgumentsWithoutStartingAttempt)
{
	spk::Client client;
	EXPECT_THROW((void)client.connect("", 1234), spk::Exception);
	EXPECT_THROW((void)client.connect("127.0.0.1", 0), spk::Exception);
	EXPECT_THROW((void)client.connect("127.0.0.1", 1234, static_cast<spk::Client::ConnectBehaviour>(99)), spk::Exception);
	EXPECT_THROW((void)client.connect("127.0.0.1", 1234, spk::Client::ConnectBehaviour::Throw, static_cast<spk::Client::ConnectAttemptBehaviour>(99)), spk::Exception);
	EXPECT_EQ(client.connection().status(), spk::Connection::Status::Disconnected);
	EXPECT_FALSE(client.isConnected());
}
