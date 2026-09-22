#include <gtest/gtest.h>

#include "network/client.hpp"
#include "network/local_node.hpp"
#include "network/node_router.hpp"
#include "network_test_utils.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace
{
	struct RoutedPayload
	{
		std::uint32_t client = 0;
		std::uint32_t sequence = 0;
	};

	void runRouter(
		spk::NodeRouter &router,
		std::atomic_bool &running,
		NetworkTestUtils::ThreadFailure &failure)
	{
		failure.run([&] {
			while (running)
			{
				router.dispatch();
				std::this_thread::sleep_for(1ms);
			}
			router.dispatch();
		});
	}

	void runEchoNode(
		spk::LocalNode &node,
		std::atomic_bool &running,
		NetworkTestUtils::ThreadFailure &failure)
	{
		failure.run([&] {
			std::vector<spk::ReceivedMessage> messages;
			while (running)
			{
				node.incoming().drain(messages);
				for (const auto &request : messages)
				{
					spk::Message response(101);
					response << request.message.peek<RoutedPayload>();
					node.reply(request, std::move(response));
				}
				std::this_thread::sleep_for(1ms);
			}
		});
	}

	void runClient(
		spk::Client &client,
		std::uint16_t port,
		std::uint32_t clientIndex,
		std::vector<spk::Message> &responses,
		NetworkTestUtils::ThreadFailure &failure)
	{
		failure.run([&] {
			client.connect("127.0.0.1", port);
			for (std::uint32_t sequence = 0; sequence < 50; ++sequence)
			{
				spk::Message request(100);
				request << RoutedPayload{clientIndex, sequence};
				client.send(request);
			}
			responses = NetworkTestUtils::collect(client.messages(), 50, 5s);
		});
	}
}

TEST(NodeRouterIntegrationTest, TwoClientsRouteThroughIndependentRouterAndNodeThreads)
{
	spk::NodeRouter router;
	spk::LocalNode node;
	spk::Client firstClient;
	spk::Client secondClient;
	NetworkTestUtils::ThreadFailure failure;
	std::atomic_bool running = true;

	router.addNode("echo", node);
	router.redirect(100, "echo");
	router.start(0);

	std::jthread routerThread([&] {
		runRouter(router, running, failure);
	});
	std::jthread nodeThread([&] {
		runEchoNode(node, running, failure);
	});

	std::array<std::vector<spk::Message>, 2> responses;
	std::jthread firstThread([&] {
		runClient(firstClient, router.server().port(), 0, responses[0], failure);
	});
	std::jthread secondThread([&] {
		runClient(secondClient, router.server().port(), 1, responses[1], failure);
	});
	firstThread.join();
	secondThread.join();

	firstClient.disconnect();
	secondClient.disconnect();
	running = false;
	nodeThread.join();
	routerThread.join();
	router.stop();

	ASSERT_NO_THROW(failure.rethrow());
	for (std::uint32_t client = 0; client < responses.size(); ++client)
	{
		ASSERT_EQ(responses[client].size(), 50u);
		for (std::uint32_t sequence = 0; sequence < responses[client].size(); ++sequence)
		{
			const RoutedPayload payload = responses[client][sequence].get<RoutedPayload>();
			EXPECT_EQ(payload.client, client);
			EXPECT_EQ(payload.sequence, sequence);
		}
	}
}

TEST(NodeRouterIntegrationTest, LocalNodeBroadcastReachesAllClients)
{
	spk::NodeRouter router;
	spk::LocalNode node;
	spk::Client firstClient;
	spk::Client secondClient;
	NetworkTestUtils::ThreadFailure failure;
	std::atomic_bool running = true;

	router.addNode("broadcast", node);
	router.start(0);
	firstClient.connect("127.0.0.1", router.server().port());
	secondClient.connect("127.0.0.1", router.server().port());

	std::jthread routerThread([&] {
		runRouter(router, running, failure);
	});

	spk::Message broadcast(102);
	broadcast << std::uint32_t{77};
	node.broadcast(std::move(broadcast));

	auto first = NetworkTestUtils::collect(firstClient.messages(), 1);
	auto second = NetworkTestUtils::collect(secondClient.messages(), 1);
	ASSERT_EQ(first.size(), 1u);
	ASSERT_EQ(second.size(), 1u);
	EXPECT_EQ(first.front().get<std::uint32_t>(), 77u);
	EXPECT_EQ(second.front().get<std::uint32_t>(), 77u);

	firstClient.disconnect();
	secondClient.disconnect();
	running = false;
	routerThread.join();
	ASSERT_NO_THROW(failure.rethrow());
	router.stop();
}
