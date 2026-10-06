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
TEST(NetworkReplicationExampleTest, RequestedObjectTraversesSerializationAndAcquisitionBarrier)
{
	const auto state = SparkleNetworkExample::requestedObject();
	EXPECT_FLOAT_EQ(state.x, 3);
	EXPECT_FLOAT_EQ(state.y, 5);
}
