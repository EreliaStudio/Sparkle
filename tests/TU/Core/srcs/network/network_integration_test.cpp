#include <gtest/gtest.h>

#include "network/client.hpp"
#include "network/server.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace
{
	template <typename TValue>
	[[nodiscard]] std::vector<TValue> waitForMessages(
		spk::ThreadSafeFIFO<TValue> &queue,
		std::chrono::milliseconds timeout = 2s)
	{
		const auto deadline = std::chrono::steady_clock::now() + timeout;
		std::vector<TValue> values;
		while (std::chrono::steady_clock::now() < deadline)
		{
			queue.drain(values);
			if (!values.empty())
			{
				return values;
			}
			std::this_thread::sleep_for(5ms);
		}
		return values;
	}
}

TEST(NetworkIntegrationTest, ClientAndServerExchangeFramedMessages)
{
	spk::Server server;
	spk::Client client;
	server.start(0);
	client.connect("127.0.0.1", server.port());

	spk::Message::Writer requestWriter(12);
	requestWriter.setRequestID(0x1020304050607080ull);
	requestWriter << std::uint32_t{42} << std::string("ping");
	client.send(std::move(requestWriter).build());

	auto receivedByServer = waitForMessages(server.messages());
	ASSERT_EQ(receivedByServer.size(), 1u);
	EXPECT_NE(receivedByServer.front().emitter, spk::InvalidConnectionID);
	EXPECT_EQ(receivedByServer.front().message.type(), 12u);
	EXPECT_EQ(receivedByServer.front().message.requestID(), 0x1020304050607080ull);

	auto requestReader = receivedByServer.front().message.reader();
	std::uint32_t value = 0;
	std::string text;
	requestReader >> value >> text;
	EXPECT_EQ(value, 42u);
	EXPECT_EQ(text, "ping");

	spk::Message::Writer responseWriter(13);
	responseWriter.setRequestID(0x8877665544332211ull);
	responseWriter << std::string("pong");
	server.sendTo(
		receivedByServer.front().emitter,
		std::move(responseWriter).build());

	auto receivedByClient = waitForMessages(client.messages());
	ASSERT_EQ(receivedByClient.size(), 1u);
	EXPECT_EQ(receivedByClient.front().type(), 13u);
	EXPECT_EQ(receivedByClient.front().requestID(), 0x8877665544332211ull);

	auto responseReader = receivedByClient.front().reader();
	std::string responseText;
	responseReader >> responseText;
	EXPECT_EQ(responseText, "pong");

	client.disconnect();
	server.stop();
}
