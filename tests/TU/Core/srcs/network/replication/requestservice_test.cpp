#include "../../../../../../examples/network_replication/requested_object.hpp"
#include "network_replication/codec.hpp"
#include "network_replication/id.hpp"
#include "network_replication/publisher.hpp"
#include <gtest/gtest.h>
#include <network/replication/protocol.hpp>
#include <network/replication/receiver.hpp>
#include <network/replication/request_queue.hpp>
#include <network/replication/request_service.hpp>
using namespace spk::Network;
using namespace std::chrono_literals;
using namespace ReplicationTest;
TEST(NetworkRequestServiceTest, ReadyWaitsUntilInitialStateIsQueuedToTransport)
{
	auto server = publisher();
	RequestService<State> service(server);
	const auto session = server.open(id(1));
	const Request request{session, id(2), 1};
	ASSERT_TRUE(service.receive(id(1), request));
	EXPECT_FALSE(service.receive(id(1), request));
	server.publish(id(2), {4});
	ASSERT_TRUE(service.accept(id(1), request));
	EXPECT_EQ(service.dispatch(10, [](auto, const auto &) {
		return true;
	}),
			  0u);
	server.dispatch({}, 10, [](auto, const auto &) {
		return false;
	});
	EXPECT_EQ(service.dispatch(10, [](auto, const auto &) {
		return true;
	}),
			  0u);
	server.dispatch({}, 10, [](auto, const auto &) {
		return true;
	});
	EXPECT_EQ(service.dispatch(10, [](auto, const auto &) {
		return true;
	}),
			  1u);
	EXPECT_EQ(service.dispatch(10, [](auto, const auto &) {
		return true;
	}),
			  0u);
}

TEST(NetworkRequestServiceTest, StaleProviderCompletionCannotAcceptNewAttempt)
{
	auto server = publisher();
	RequestService<State> service(server);
	const auto session = server.open(id(1));
	const Request first{session, id(2), 1}, second{session, id(2), 2};
	ASSERT_TRUE(service.receive(id(1), first));
	ASSERT_TRUE(service.receive(id(1), second));
	EXPECT_TRUE(service.fulfill(id(1), second, State{4}));
	EXPECT_FALSE(service.fulfill(id(1), first, State{99}));
	server.dispatch({}, 10, [](auto, const auto &update) {
		EXPECT_EQ(update.state->value, 4);
		return true;
	});
	service.close(id(1));
	server.close(id(1));
	(void)server.open(id(1));
	EXPECT_FALSE(service.receive(id(1), second));
}
