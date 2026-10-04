#include <gtest/gtest.h>

#include "network/client.hpp"
#include "network/node_router.hpp"
#include "network/remote_node.hpp"
#include "network/server.hpp"
#include "network_test_utils.hpp"

#include <array>
#include <cstdint>
#include <thread>
#include <vector>

namespace
{
	spk::Message request(std::uint32_t value)
	{
		spk::Message::Writer writer(70);
		writer.setRequestID(123);
		writer << value;
		return std::move(writer).build();
	}

	spk::Message response(const spk::Message &message)
	{
		spk::Message::Writer writer(71);
		writer.setRequestID(message.requestID());
		writer << message.reader().get<std::uint32_t>();
		return std::move(writer).build();
	}

	struct Receiver
	{
		std::vector<std::uint32_t> values;
		const std::thread::id thread = std::this_thread::get_id();
		spk::Client::MessageDispatcher::Contract contract;

		explicit Receiver(spk::Client &client) :
			contract(client.messageDispatcher().subscribeTo(71, [this](const spk::Message &message) {
				EXPECT_EQ(std::this_thread::get_id(), thread);
				EXPECT_EQ(message.requestID(), 123u);
				values.push_back(message.reader().get<std::uint32_t>());
			}))
		{
		}
	};
}

TEST(MessageDispatcherIntegrationTest, DirectServerRepliesReachOnlyTheirEmitter)
{
	spk::Server server;
	spk::Client first, second;
	Receiver firstReceiver(first), secondReceiver(second);
	std::vector<spk::ConnectionID> emitters;
	const auto owner = std::this_thread::get_id();
	auto contract = server.messageDispatcher().subscribeTo(70, [&](const spk::ReceivedMessage &received) {
		EXPECT_EQ(std::this_thread::get_id(), owner);
		emitters.push_back(received.emitter);
		server.sendTo(received.emitter, response(received.message));
	});
	server.start(0);
	first.connect("127.0.0.1", server.port());
	second.connect("127.0.0.1", server.port());
	first.send(request(1));
	second.send(request(2));
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		server.treatMessages();
		first.treatMessages();
		second.treatMessages();
		return firstReceiver.values.size() == 1 && secondReceiver.values.size() == 1;
	}));
	ASSERT_EQ(emitters.size(), 2u);
	EXPECT_NE(emitters[0], emitters[1]);
	EXPECT_EQ(firstReceiver.values, (std::vector<std::uint32_t>{1}));
	EXPECT_EQ(secondReceiver.values, (std::vector<std::uint32_t>{2}));
	first.disconnect();
	second.disconnect();
	server.stop();
}

TEST(MessageDispatcherIntegrationTest, RoutedDelayedRepliesPreserveProxyAndOrigin)
{
	spk::RemoteNode::Endpoint endpoint;
	std::array<spk::RemoteNode, 2> remotes;
	std::array<spk::NodeRouter, 2> routers;
	std::array<spk::Client, 2> clients;
	Receiver first(clients[0]), second(clients[1]);
	std::vector<spk::RemoteNode::Endpoint::Request> requests;
	auto contract = endpoint.messageDispatcher().subscribeTo(70, [&](const auto &incoming) {
		requests.push_back(incoming);
	});
	endpoint.start(0);
	for (std::size_t index = 0; index < clients.size(); ++index)
	{
		remotes[index].connect("127.0.0.1", endpoint.port());
		routers[index].addNode("remote", remotes[index]);
		routers[index].redirect(70, "remote");
		routers[index].start(0);
		clients[index].connect("127.0.0.1", routers[index].server().port());
		clients[index].send(request(static_cast<std::uint32_t>(index + 1)));
	}
	auto pump = [&] {
		for (auto &router : routers)
		{
			router.dispatch();
		}
		for (auto &remote : remotes)
		{
			remote.dispatch();
		}
		endpoint.dispatch();
		endpoint.treatMessages();
		for (auto &client : clients)
		{
			client.treatMessages();
		}
	};
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return requests.size() == 2;
	}));
	EXPECT_NE(requests[0].proxyConnection, requests[1].proxyConnection);
	EXPECT_EQ(requests[0].originConnection, requests[1].originConnection);
	for (auto iterator = requests.rbegin(); iterator != requests.rend(); ++iterator)
	{
		endpoint.reply(*iterator, response(iterator->message));
	}
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return first.values.size() == 1 && second.values.size() == 1;
	}));
	EXPECT_EQ(first.values, (std::vector<std::uint32_t>{1}));
	EXPECT_EQ(second.values, (std::vector<std::uint32_t>{2}));
	for (auto &client : clients)
	{
		client.disconnect();
	}
	for (auto &remote : remotes)
	{
		remote.disconnect();
	}
	for (auto &router : routers)
	{
		router.stop();
	}
	endpoint.stop();
}
