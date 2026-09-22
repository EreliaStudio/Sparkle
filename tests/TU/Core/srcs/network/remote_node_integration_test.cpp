#include <gtest/gtest.h>

#include "exception.hpp"
#include "network/client.hpp"
#include "network/node_router.hpp"
#include "network/remote_node.hpp"
#include "network/remote_node_endpoint.hpp"
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
	struct RemotePayload
	{
		std::uint32_t client = 0;
		std::uint32_t sequence = 0;
	};

	void runRemoteRouter(
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

	void runRemoteEndpoint(
		spk::RemoteNodeEndpoint &endpoint,
		std::size_t expectedRequests,
		std::atomic_bool &finished,
		NetworkTestUtils::ThreadFailure &failure)
	{
		failure.run([&] {
			std::vector<spk::RemoteRequest> requests;
			std::vector<spk::RemoteRequest> batch;
			while (requests.size() < expectedRequests)
			{
				endpoint.dispatch();
				endpoint.requests().drain(batch);
				for (spk::RemoteRequest &request : batch)
				{
					requests.push_back(std::move(request));
				}
				std::this_thread::sleep_for(1ms);
			}

			for (auto iterator = requests.rbegin(); iterator != requests.rend(); ++iterator)
			{
				spk::Message response(201);
				response << iterator->message.peek<RemotePayload>();
				endpoint.reply(*iterator, std::move(response));
			}
			finished = true;
		});
	}

	void runRemoteClient(
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
				spk::Message request(200);
				request << RemotePayload{clientIndex, sequence};
				client.send(request);
			}
			responses = NetworkTestUtils::collect(client.messages(), 50, 5s);
		});
	}
}

TEST(RemoteNodeIntegrationTest, TwoClientsRemainCorrelatedAcrossOutOfOrderRemoteReplies)
{
	constexpr std::size_t RequestCount = 100;
	spk::RemoteNodeEndpoint endpoint;
	spk::RemoteNode remoteNode;
	spk::NodeRouter router;
	spk::Client firstClient;
	spk::Client secondClient;
	NetworkTestUtils::ThreadFailure failure;
	std::atomic_bool routerRunning = true;
	std::atomic_bool endpointFinished = false;

	endpoint.start(0);
	remoteNode.connect("127.0.0.1", endpoint.port());
	router.addNode("remote", remoteNode);
	router.redirect(200, "remote");
	router.start(0);

	std::jthread routerThread([&] {
		runRemoteRouter(router, routerRunning, failure);
	});
	std::jthread endpointThread([&] {
		runRemoteEndpoint(endpoint, RequestCount, endpointFinished, failure);
	});

	std::array<std::vector<spk::Message>, 2> responses;
	std::jthread firstThread([&] {
		runRemoteClient(firstClient, router.server().port(), 0, responses[0], failure);
	});
	std::jthread secondThread([&] {
		runRemoteClient(secondClient, router.server().port(), 1, responses[1], failure);
	});
	firstThread.join();
	secondThread.join();

	ASSERT_NO_THROW(failure.rethrow());
	ASSERT_TRUE(endpointFinished);
	for (std::uint32_t client = 0; client < responses.size(); ++client)
	{
		ASSERT_EQ(responses[client].size(), 50u);
		for (spk::Message &message : responses[client])
		{
			const RemotePayload payload = message.get<RemotePayload>();
			EXPECT_EQ(payload.client, client);
			EXPECT_LT(payload.sequence, 50u);
		}
	}

	firstClient.disconnect();
	secondClient.disconnect();
	routerRunning = false;
	routerThread.join();
	endpointThread.join();
	remoteNode.disconnect();
	router.stop();
	endpoint.stop();
}

TEST(RemoteNodeIntegrationTest, ForwardingWhileDisconnectedThrows)
{
	spk::RemoteNode remoteNode;

	EXPECT_THROW(
		remoteNode.receive(spk::ReceivedMessage{12, spk::Message(200)}),
		spk::Exception);
}

TEST(RemoteNodeIntegrationTest, EndpointRejectsOrdinaryClientMessages)
{
	spk::RemoteNodeEndpoint endpoint;
	spk::Client client;
	endpoint.start(0);
	client.connect("127.0.0.1", endpoint.port());
	client.send(spk::Message(77));

	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		try
		{
			endpoint.dispatch();
			return false;
		}
		catch (const spk::Exception &)
		{
			return true;
		}
	}));

	client.disconnect();
	endpoint.stop();
}
