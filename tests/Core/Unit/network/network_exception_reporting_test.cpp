#include "diagnostics/logger.hpp"
#include "exception.hpp"
#include "network/client.hpp"
#include "network/internal/transfer_completion.hpp"
#include "network/server.hpp"
#include "network_test_utils.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>
#include <string>

using namespace std::chrono_literals;

TEST(NetworkExceptionReporting, CompletionFailureLogsCauseOnceAndKeepsTerminalOutcome)
{
	spk::ThreadSafeFIFO<std::string> entries;
	auto logging = spk::logger.subscribeToEntry([&](const spk::Logger::Level &level, const std::string &message) {
		if (level == spk::Logger::Level::Error && message.find("completion regression cause") != std::string::npos)
		{
			entries.publish(message);
		}
	});
	spk::Transfer transfer;
	auto completion = transfer.subscribeToCompletion([](auto, auto) {
		try
		{
			throw std::runtime_error("completion regression cause");
		} catch (...)
		{
			throw spk::Exception("completion callback context", std::current_exception());
		}
	});
	EXPECT_TRUE(spk::NetworkInternal::TransferCompletion::complete(transfer, spk::Transfer::Status::Success));
	EXPECT_FALSE(spk::NetworkInternal::TransferCompletion::complete(transfer, spk::Transfer::Status::Failed));
	EXPECT_EQ(transfer.status(), spk::Transfer::Status::Success);
	const auto logged = NetworkTestUtils::collect(entries, 1);
	ASSERT_EQ(logged.size(), 1u);
	EXPECT_NE(logged.front().find("Network transfer completion callback failed."), std::string::npos);
	EXPECT_NE(logged.front().find("completion callback context"), std::string::npos);
}

TEST(NetworkExceptionReporting, ServerCallbackFailureLogsOnceAndLeavesConnectionUsable)
{
	spk::ThreadSafeFIFO<std::string> entries;
	auto logging = spk::logger.subscribeToEntry([&](const spk::Logger::Level &level, const std::string &message) {
		if (level == spk::Logger::Level::Error && message.find("server callback regression cause") != std::string::npos)
		{
			entries.publish(message);
		}
	});
	spk::Server server;
	spk::Client client;
	auto status = server.subscribeToConnectionStatusChange([](auto, spk::Connection::Status state, auto) {
		if (state == spk::Connection::Status::Connected)
		{
			throw std::runtime_error("server callback regression cause");
		}
	});
	server.start(0);
	client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));
	const auto logged = NetworkTestUtils::collect(entries, 1, 3s);
	ASSERT_EQ(logged.size(), 1u);
	EXPECT_NE(logged.front().find("Network server Connected callback failed."), std::string::npos);
	const auto transfer = client.send(spk::Message::Writer(281).build());
	EXPECT_EQ(NetworkTestUtils::collect(server.messages(), 1, 3s).size(), 1u);
	EXPECT_TRUE(NetworkTestUtils::waitUntil([&] {
		return transfer.isComplete();
	}));
	EXPECT_EQ(transfer.status(), spk::Transfer::Status::Success);
}

TEST(NetworkExceptionReporting, ClientCallbackFailureLogsOnceAndReportsDisconnection)
{
	spk::ThreadSafeFIFO<std::string> entries;
	auto logging = spk::logger.subscribeToEntry([&](const spk::Logger::Level &level, const std::string &message) {
		if (level == spk::Logger::Level::Error && message.find("client callback regression cause") != std::string::npos)
		{
			entries.publish(message);
		}
	});
	spk::Server server;
	spk::Client client;
	auto status = client.subscribeToConnectionStatusChange([](auto, spk::Connection::Status state, auto) {
		if (state == spk::Connection::Status::Connected)
		{
			throw std::runtime_error("client callback regression cause");
		}
	});
	server.start(0);
	const auto connection = client.connect("127.0.0.1", server.port());
	const auto logged = NetworkTestUtils::collect(entries, 1, 3s);
	ASSERT_EQ(logged.size(), 1u);
	EXPECT_NE(logged.front().find("Network client connection startup or Connected callback failed."), std::string::npos);
	EXPECT_TRUE(NetworkTestUtils::waitUntil([&] {
		return connection.status() == spk::Connection::Status::Disconnected;
	}));
	EXPECT_EQ(connection.error(), spk::Connection::Error::TransportFailure);
}
