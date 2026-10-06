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
TEST(NetworkRequestQueueTest, FailedAcquisitionRetriesWithoutApplicationMovement)
{
	RequestQueue requests;
	(void)requests.reset(id(1));
	requests.request(id(2), {});
	const auto first = requests.due({}).at(0);
	EXPECT_TRUE(requests.receive({first, Reply::Result::Retry}, {}));
	EXPECT_TRUE(requests.due(Clock::time_point{} + 999ms).empty());
	const auto second = requests.due(Clock::time_point{} + 1s).at(0);
	EXPECT_NE(first.attempt, second.attempt);
	EXPECT_FALSE(requests.receive({first}, Clock::time_point{} + 1s));
	EXPECT_TRUE(requests.receive({second}, Clock::time_point{} + 1s));
	EXPECT_EQ(requests.status(id(2)), RequestQueue::Status::Ready);
}

TEST(NetworkRequestQueueTest, TimeoutBackoffPermanentRefusalAndCancellation)
{
	RequestQueue requests;
	(void)requests.reset(id(1));
	requests.request(id(2), {});
	const auto first = requests.due({}).at(0);
	EXPECT_TRUE(requests.due(Clock::time_point{} + 15s).empty());
	const auto second = requests.due(Clock::time_point{} + 16s).at(0);
	EXPECT_TRUE(
		requests.receive({second, Reply::Result::Retry}, Clock::time_point{} + 16s));
	EXPECT_TRUE(requests.due(Clock::time_point{} + 17s).empty());
	const auto third = requests.due(Clock::time_point{} + 18s).at(0);
	EXPECT_TRUE(
		requests.receive({third, Reply::Result::Rejected}, Clock::time_point{} + 18s));
	EXPECT_TRUE(requests.due(Clock::time_point{} + 100s).empty());
	EXPECT_EQ(requests.status(id(2)), RequestQueue::Status::Failed);
	requests.release(id(2));
	requests.request(id(2), Clock::time_point{} + 101s);
	EXPECT_GT(requests.due(Clock::time_point{} + 101s).at(0).attempt, first.attempt);
	requests.release(id(2));
	EXPECT_TRUE(requests.due(Clock::time_point{} + 200s).empty());
}

TEST(NetworkRequestQueueTest, RetryLimitStopsRepeatedFailures)
{
	RequestQueue requests({.maximumAttempts = 2});
	(void)requests.reset(id(1));
	requests.request(id(2), {});
	auto attempt = requests.due({}).at(0);
	EXPECT_TRUE(requests.receive({attempt, Reply::Result::Retry}, {}));
	attempt = requests.due(Clock::time_point{} + 1s).at(0);
	EXPECT_TRUE(
		requests.receive({attempt, Reply::Result::Retry}, Clock::time_point{} + 1s));
	EXPECT_EQ(requests.status(id(2)), RequestQueue::Status::Failed);
}
