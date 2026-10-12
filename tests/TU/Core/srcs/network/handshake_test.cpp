#include "network/internal/handshake.hpp"
#include "network/internal/handshake_deadline.hpp"

#include "exception.hpp"

#include <gtest/gtest.h>

#include <chrono>

namespace
{
	using spk::NetworkInternal::Handshake;
	using spk::NetworkInternal::HandshakeDeadline;
}

TEST(Handshake, HelloAndAcceptanceEncodeIndependentProtocolVersion)
{
	const auto hello = Handshake::decode(*Handshake::encode(Handshake::Kind::Hello));
	EXPECT_TRUE(Handshake::validHello(hello));
	EXPECT_EQ(hello.connectionID, spk::Connection::InvalidID);
	const auto accepted = Handshake::decode(*Handshake::encode(Handshake::Kind::Accepted, 71));
	EXPECT_TRUE(Handshake::validAcceptance(accepted));
	EXPECT_EQ(accepted.connectionID, 71u);
}

TEST(Handshake, WrongMagicAndVersionProduceDedicatedErrors)
{
	auto hello = Handshake::encode(Handshake::Kind::Hello);
	(*hello)[32] = std::byte{'X'};
	EXPECT_EQ(Handshake::decode(*hello).validationError, spk::Connection::Error::InvalidMagic);
	(*hello)[32] = std::byte{'S'};
	(*hello)[37] = std::byte{2};
	EXPECT_EQ(Handshake::decode(*hello).validationError, spk::Connection::Error::VersionMismatch);
}

TEST(Handshake, ExplicitRejectionCarriesFailureReason)
{
	const auto rejection = Handshake::decode(*Handshake::encode(
		Handshake::Kind::Rejected, 17, spk::Connection::Error::VersionMismatch));
	EXPECT_EQ(rejection.error, spk::Connection::Error::VersionMismatch);
	EXPECT_FALSE(Handshake::validAcceptance(rejection));
	EXPECT_THROW(Handshake::encode(Handshake::Kind::Accepted, 0), spk::Exception);
}

TEST(Handshake, DeadlineIsRelativeToStartNotEachPage)
{
	using namespace std::chrono_literals;
	const auto now = HandshakeDeadline::Clock::now();
	HandshakeDeadline deadline(5s, now);
	EXPECT_FALSE(deadline.expired(now + 4999ms));
	EXPECT_TRUE(deadline.expired(now + 5s));
	EXPECT_EQ(deadline.remaining(now + 9s), 0ms);
	EXPECT_THROW(HandshakeDeadline(0ms), spk::Exception);
}
