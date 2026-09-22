#include <gtest/gtest.h>

#include "exception.hpp"
#include "network/local_node.hpp"
#include "network/node_router.hpp"

#include <vector>

TEST(NodeRouterTest, RoutesIncomingMessageByType)
{
	spk::NodeRouter router;
	spk::LocalNode node;
	router.addNode("game", node);
	router.redirect(17, "game");
	router.server().messages().publish(
		spk::ReceivedMessage{91, spk::Message(17)});

	router.dispatch();

	std::vector<spk::ReceivedMessage> received;
	node.incoming().drain(received);
	ASSERT_EQ(received.size(), 1u);
	EXPECT_EQ(received.front().emitter, 91u);
	EXPECT_EQ(received.front().message.type(), 17u);
}

TEST(NodeRouterTest, RejectsDuplicateNodeName)
{
	spk::NodeRouter router;
	spk::LocalNode first;
	spk::LocalNode second;
	router.addNode("game", first);

	EXPECT_THROW(router.addNode("game", second), spk::Exception);
}

TEST(NodeRouterTest, RejectsUnknownRedirectionTarget)
{
	spk::NodeRouter router;

	EXPECT_THROW(router.redirect(1, "missing"), spk::Exception);
}

TEST(NodeRouterTest, RejectsUnroutedIncomingMessage)
{
	spk::NodeRouter router;
	router.server().messages().publish(
		spk::ReceivedMessage{3, spk::Message(99)});

	EXPECT_THROW(router.dispatch(), spk::Exception);
}

TEST(NodeRouterTest, RemovingNodeAlsoRemovesItsRoutes)
{
	spk::NodeRouter router;
	spk::LocalNode node;
	router.addNode("game", node);
	router.redirect(4, "game");
	router.removeNode("game");
	router.server().messages().publish(
		spk::ReceivedMessage{3, spk::Message(4)});

	EXPECT_THROW(router.dispatch(), spk::Exception);
}

TEST(NodeRouterTest, RejectsSameNodeRegisteredUnderDifferentNames)
{
	spk::NodeRouter router;
	spk::LocalNode node;
	router.addNode("first", node);

	EXPECT_THROW(router.addNode("second", node), spk::Exception);
}

TEST(NodeRouterTest, RedirectCanBeReassignedToAnotherNode)
{
	spk::NodeRouter router;
	spk::LocalNode first;
	spk::LocalNode second;
	router.addNode("first", first);
	router.addNode("second", second);
	router.redirect(8, "first");
	router.redirect(8, "second");
	router.server().messages().publish(
		spk::ReceivedMessage{5, spk::Message(8)});

	router.dispatch();

	std::vector<spk::ReceivedMessage> firstMessages;
	std::vector<spk::ReceivedMessage> secondMessages;
	first.incoming().drain(firstMessages);
	second.incoming().drain(secondMessages);
	EXPECT_TRUE(firstMessages.empty());
	ASSERT_EQ(secondMessages.size(), 1u);
	EXPECT_EQ(secondMessages.front().emitter, 5u);
}
