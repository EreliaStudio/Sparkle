#include <gtest/gtest.h>

#include "network/local_node.hpp"

#include <cstdint>
#include <vector>

TEST(LocalNodeTest, ReceivePublishesIncomingMessage)
{
	spk::LocalNode node;
	node.receive(spk::ReceivedMessage{41, spk::Message(7)});

	std::vector<spk::ReceivedMessage> received;
	node.incoming().drain(received);

	ASSERT_EQ(received.size(), 1u);
	EXPECT_EQ(received.front().emitter, 41u);
	EXPECT_EQ(received.front().message.type(), 7u);
}

TEST(LocalNodeTest, ReplyTargetsRequestEmitter)
{
	spk::LocalNode node;
	const spk::ReceivedMessage request{52, spk::Message(1)};

	node.reply(request, spk::Message(2));

	std::vector<spk::OutgoingMessage> outgoing;
	node.outgoing().drain(outgoing);

	ASSERT_EQ(outgoing.size(), 1u);
	ASSERT_TRUE(outgoing.front().recipient.has_value());
	EXPECT_EQ(*outgoing.front().recipient, 52u);
	EXPECT_EQ(outgoing.front().message.type(), 2u);
}

TEST(LocalNodeTest, BroadcastHasNoRecipient)
{
	spk::LocalNode node;
	node.broadcast(spk::Message(5));

	std::vector<spk::OutgoingMessage> outgoing;
	node.outgoing().drain(outgoing);

	ASSERT_EQ(outgoing.size(), 1u);
	EXPECT_FALSE(outgoing.front().recipient.has_value());
	EXPECT_EQ(outgoing.front().message.type(), 5u);
}
