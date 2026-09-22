#include <gtest/gtest.h>

#include "network/client.hpp"
#include "network/server.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
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

	spk::Message request(12);
	request << std::uint32_t{42} << std::string("ping");
	client.send(request);

	auto receivedByServer = waitForMessages(server.messages());
	ASSERT_EQ(receivedByServer.size(), 1u);
	EXPECT_NE(receivedByServer.front().emitter, spk::InvalidConnectionID);
	EXPECT_EQ(receivedByServer.front().message.type(), 12u);

	std::uint32_t value = 0;
	std::string text;
	receivedByServer.front().message >> value >> text;
	EXPECT_EQ(value, 42u);
	EXPECT_EQ(text, "ping");

	spk::Message response(13);
	response << std::string("pong");
	server.sendTo(receivedByServer.front().emitter, response);

	auto receivedByClient = waitForMessages(client.messages());
	ASSERT_EQ(receivedByClient.size(), 1u);
	EXPECT_EQ(receivedByClient.front().type(), 13u);

	std::string responseText;
	receivedByClient.front() >> responseText;
	EXPECT_EQ(responseText, "pong");

	client.disconnect();
	server.stop();
}
