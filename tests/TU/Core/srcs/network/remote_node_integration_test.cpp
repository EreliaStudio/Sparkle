#include <gtest/gtest.h>

#include "exception.hpp"
#include "network/client.hpp"
#include "network/node_router.hpp"
#include "network/remote_node.hpp"
#include "network_test_utils.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace
{
	struct RemotePayload
	{
		std::uint32_t client = 0;
		std::uint32_t sequence = 0;
	};

	[[nodiscard]] spk::Message::RequestID requestID(
		std::uint32_t client,
		std::uint32_t sequence)
	{
		return (static_cast<spk::Message::RequestID>(client) + 1u) * 1000u + sequence;
	}

	[[nodiscard]] spk::Message payloadMessage(
		spk::Message::Type type,
		const RemotePayload &payload,
		spk::Message::RequestID id = 0)
	{
		spk::Message::Writer writer(type);
		writer.setRequestID(id);
		writer << payload;
		return std::move(writer).build();
	}

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

	void runRemoteNode(
		spk::RemoteNode &remoteNode,
		std::atomic_bool &running,
		NetworkTestUtils::ThreadFailure &failure)
	{
		failure.run([&] {
			while (running)
			{
				remoteNode.dispatch();
				std::this_thread::sleep_for(1ms);
			}
			remoteNode.dispatch();
		});
	}

	void runRemoteEndpoint(
		spk::RemoteNode::Endpoint &endpoint,
		std::size_t expectedRequests,
		std::atomic_bool &finished,
		NetworkTestUtils::ThreadFailure &failure)
	{
		failure.run([&] {
			std::vector<spk::RemoteNode::Endpoint::Request> requests;
			std::vector<spk::RemoteNode::Endpoint::Request> batch;
			const auto deadline = std::chrono::steady_clock::now() + 5s;
			while (
				requests.size() < expectedRequests &&
				std::chrono::steady_clock::now() < deadline)
			{
				endpoint.dispatch();
				endpoint.requests().drain(batch);
				for (spk::RemoteNode::Endpoint::Request &request : batch)
				{
					requests.push_back(std::move(request));
				}
				std::this_thread::sleep_for(1ms);
			}
			if (requests.size() != expectedRequests)
			{
				throw spk::Exception("Timed out waiting for remote requests.");
			}

			for (auto iterator = requests.rbegin(); iterator != requests.rend(); ++iterator)
			{
				const RemotePayload payload =
					iterator->message.reader().get<RemotePayload>();
				if (iterator->message.requestID() != requestID(payload.client, payload.sequence))
				{
					throw spk::Exception("Remote request ID was not preserved.");
				}
				endpoint.reply(
					*iterator,
					payloadMessage(
						201,
						payload,
						iterator->message.requestID()));
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
				client.send(
					payloadMessage(
						200,
						RemotePayload{clientIndex, sequence},
						requestID(clientIndex, sequence)));
			}
			responses = NetworkTestUtils::collect(client.messages(), 50, 5s);
		});
	}
}

TEST(RemoteNodeIntegrationTest, TwoClientsRemainCorrelatedAcrossOutOfOrderRemoteReplies)
{
	constexpr std::size_t RequestCount = 100;
	spk::RemoteNode::Endpoint endpoint;
	spk::RemoteNode remoteNode;
	spk::NodeRouter router;
	spk::Client firstClient;
	spk::Client secondClient;
	NetworkTestUtils::ThreadFailure failure;
	std::atomic_bool routerRunning = true;
	std::atomic_bool remoteNodeRunning = true;
	std::atomic_bool endpointFinished = false;

	endpoint.start(0);
	remoteNode.connect("127.0.0.1", endpoint.port());
	router.addNode("remote", remoteNode);
	router.redirect(200, "remote");
	router.start(0);

	std::jthread routerThread([&] {
		runRemoteRouter(router, routerRunning, failure);
	});
	std::jthread remoteNodeThread([&] {
		runRemoteNode(remoteNode, remoteNodeRunning, failure);
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
	endpointThread.join();

	firstClient.disconnect();
	secondClient.disconnect();
	routerRunning = false;
	routerThread.join();
	remoteNodeRunning = false;
	remoteNodeThread.join();
	remoteNode.disconnect();
	router.stop();
	endpoint.stop();

	ASSERT_NO_THROW(failure.rethrow());
	ASSERT_TRUE(endpointFinished);
	for (std::uint32_t client = 0; client < responses.size(); ++client)
	{
		ASSERT_EQ(responses[client].size(), 50u);
		std::array<bool, 50> seen{};
		for (const spk::Message &message : responses[client])
		{
			const RemotePayload payload =
				message.reader().get<RemotePayload>();
			EXPECT_EQ(payload.client, client);
			EXPECT_EQ(message.requestID(), requestID(payload.client, payload.sequence));
			ASSERT_LT(payload.sequence, seen.size());
			EXPECT_FALSE(seen[payload.sequence]);
			seen[payload.sequence] = true;
		}
		EXPECT_TRUE(std::all_of(seen.begin(), seen.end(), [](bool value) {
			return value;
		}));
	}
}

TEST(RemoteNodeIntegrationTest, ForwardingWhileDisconnectedThrows)
{
	spk::RemoteNode remoteNode;

	EXPECT_THROW(
		(remoteNode.receive(
			 spk::ReceivedMessage{
				 12,
				 spk::Message::Writer(200).build()}),
		 remoteNode.dispatch()),
		spk::Exception);
}

TEST(RemoteNodeIntegrationTest, EndpointRejectsOrdinaryClientMessages)
{
	spk::RemoteNode::Endpoint endpoint;
	spk::Client client;
	endpoint.start(0);
	client.connect("127.0.0.1", endpoint.port());
	client.send(spk::Message::Writer(77).build());

	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		try
		{
			endpoint.dispatch();
			return false;
		} catch (const spk::Exception &)
		{
			return true;
		}
	}));

	client.disconnect();
	endpoint.stop();
}

TEST(RemoteNodeIntegrationTest, TwoRoutersSharingEndpointKeepOriginConnectionsIndependent)
{
	spk::RemoteNode::Endpoint endpoint;
	spk::RemoteNode firstRemote;
	spk::RemoteNode secondRemote;
	spk::NodeRouter firstRouter;
	spk::NodeRouter secondRouter;
	spk::Client firstClient;
	spk::Client secondClient;
	NetworkTestUtils::ThreadFailure failure;
	std::atomic_bool running = true;

	endpoint.start(0);
	firstRemote.connect("127.0.0.1", endpoint.port());
	secondRemote.connect("127.0.0.1", endpoint.port());

	firstRouter.addNode("remote", firstRemote);
	firstRouter.redirect(220, "remote");
	firstRouter.start(0);
	secondRouter.addNode("remote", secondRemote);
	secondRouter.redirect(220, "remote");
	secondRouter.start(0);

	std::jthread firstRouterThread([&] {
		runRemoteRouter(firstRouter, running, failure);
	});
	std::jthread secondRouterThread([&] {
		runRemoteRouter(secondRouter, running, failure);
	});
	std::jthread firstRemoteThread([&] {
		runRemoteNode(firstRemote, running, failure);
	});
	std::jthread secondRemoteThread([&] {
		runRemoteNode(secondRemote, running, failure);
	});

	firstClient.connect("127.0.0.1", firstRouter.server().port());
	secondClient.connect("127.0.0.1", secondRouter.server().port());

	firstClient.send(payloadMessage(220, RemotePayload{10, 1}));
	secondClient.send(payloadMessage(220, RemotePayload{20, 2}));

	std::vector<spk::RemoteNode::Endpoint::Request> requests;
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		endpoint.dispatch();
		std::vector<spk::RemoteNode::Endpoint::Request> batch;
		endpoint.requests().drain(batch);
		for (auto &request : batch)
		{
			requests.push_back(std::move(request));
		}
		return requests.size() == 2;
	},
											5s));

	ASSERT_EQ(requests.size(), 2u);
	EXPECT_NE(requests[0].proxyConnection, requests[1].proxyConnection);
	EXPECT_EQ(requests[0].originConnection, requests[1].originConnection);

	for (const auto &request : requests)
	{
		const RemotePayload payload =
			request.message.reader().get<RemotePayload>();
		endpoint.reply(request, payloadMessage(221, payload));
	}

	auto firstResponses = NetworkTestUtils::collect(firstClient.messages(), 1, 5s);
	auto secondResponses = NetworkTestUtils::collect(secondClient.messages(), 1, 5s);
	ASSERT_EQ(firstResponses.size(), 1u);
	ASSERT_EQ(secondResponses.size(), 1u);
	EXPECT_EQ(
		firstResponses.front().reader().get<RemotePayload>().client,
		10u);
	EXPECT_EQ(
		secondResponses.front().reader().get<RemotePayload>().client,
		20u);

	firstClient.disconnect();
	secondClient.disconnect();
	running = false;
	firstRouterThread.join();
	secondRouterThread.join();
	firstRemoteThread.join();
	secondRemoteThread.join();
	firstRemote.disconnect();
	secondRemote.disconnect();
	firstRouter.stop();
	secondRouter.stop();
	endpoint.stop();
	ASSERT_NO_THROW(failure.rethrow());
}
