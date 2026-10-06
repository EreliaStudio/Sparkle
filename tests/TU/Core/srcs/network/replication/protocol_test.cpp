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
TEST(NetworkReplicationProtocolTest, DecodesStateOnceAndRoundTripsControl)
{
	Protocol<State, Codec> protocol(42);
	Codec::decodes = 0;
	auto frame = protocol.encode(
		Update<State>{id(1), id(2), 1, 1, Edit::Set, std::make_shared<State>(State{5})});
	EXPECT_EQ(protocol.kind(frame), (Protocol<State, Codec>::Kind::Update));
	Receiver<State> receiver;
	receiver.reset(id(1));
	auto update = receiver.receive(protocol.decodeUpdate(frame));
	ASSERT_TRUE(update);
	EXPECT_EQ(update->state->value, 5);
	EXPECT_EQ(Codec::decodes, 1);
	const Request request{id(1), id(2), 3};
	EXPECT_EQ(protocol.decodeRequest(protocol.encode(request)), request);
	const auto reply =
		protocol.decodeReply(protocol.encode(Reply{request, Reply::Result::Retry}));
	EXPECT_EQ(reply.request, request);
	EXPECT_EQ(reply.result, Reply::Result::Retry);
}

TEST(NetworkReplicationProtocolTest, RejectsEveryTruncationAndTrailingData)
{
	Protocol<State, Codec> protocol(42);
	auto frame = protocol.encode(
		Update<State>{id(1), id(2), 1, 1, Edit::Set, std::make_shared<State>(State{5})});
	for (std::size_t size = 0; size < frame.size(); ++size)
	{
		spk::Message::Writer writer(42);
		writer.append(frame.data().data(), size);
		EXPECT_THROW((void)protocol.decodeUpdate(std::move(writer).build()), spk::Exception);
	}
	spk::Message::Writer writer(42);
	writer.append(frame.data().data(), frame.size());
	writer << 1;
	EXPECT_THROW((void)protocol.decodeUpdate(std::move(writer).build()), spk::Exception);
}
