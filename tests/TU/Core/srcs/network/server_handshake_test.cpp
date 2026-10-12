#include "network/internal/server_handshake.hpp"

#include "exception.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

TEST(ServerHandshake, AcceptanceRequiresCompletedControlWrite)
{
	spk::NetworkInternal::ServerHandshake handshake;
	const auto hello = spk::NetworkInternal::Handshake::encode(spk::NetworkInternal::Handshake::Kind::Hello);
	EXPECT_EQ(handshake.accept(*hello), spk::NetworkInternal::Handshake::Kind::Accepted);
	EXPECT_FALSE(handshake.ready());
	EXPECT_TRUE(handshake.written(spk::NetworkInternal::Handshake::Kind::Accepted));
	EXPECT_TRUE(handshake.ready());
	EXPECT_THROW(handshake.accept(*hello), spk::Exception);
}

TEST(ServerHandshake, InvalidVersionIsExplicitlyRejected)
{
	spk::NetworkInternal::ServerHandshake handshake;
	auto hello = spk::NetworkInternal::Handshake::encode(spk::NetworkInternal::Handshake::Kind::Hello);
	(*hello)[37] = std::byte{2};
	EXPECT_EQ(handshake.accept(*hello), spk::NetworkInternal::Handshake::Kind::Rejected);
	EXPECT_EQ(handshake.error(), spk::Connection::Error::VersionMismatch);
	EXPECT_FALSE(handshake.written(spk::NetworkInternal::Handshake::Kind::Rejected));
	EXPECT_EQ(handshake.state(), spk::NetworkInternal::ServerHandshake::State::Rejected);
}

TEST(ServerHandshake, DeadlineAndFailureStayPerSession)
{
	spk::NetworkInternal::ServerHandshake handshake(1ms);
	std::this_thread::sleep_for(2ms);
	EXPECT_TRUE(handshake.expired());
	handshake.fail(spk::Connection::Error::Timeout);
	EXPECT_EQ(handshake.error(), spk::Connection::Error::Timeout);
}
