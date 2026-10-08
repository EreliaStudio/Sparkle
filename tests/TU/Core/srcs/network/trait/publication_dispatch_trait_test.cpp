#include "network/network_traits_test.hpp"
using namespace std::chrono_literals;

class PublicationDispatchTraitTest : public NetworkTraitsTest
{
protected:
	class Queue : public spk::Network::PublicationQueueTrait
	{
	public:
		using PublicationQueueTrait::_closeQueue;
		using PublicationQueueTrait::_front;
		using PublicationQueueTrait::_openQueue;
		using PublicationQueueTrait::_pop;
		using PublicationQueueTrait::_queue;
		using PublicationQueueTrait::PublicationQueueTrait;
	};
	class Dispatcher : public spk::Network::PublicationDispatchTrait
	{
		void _capturePublicationChanges() override
		{
			++captures;
		}
		bool _sendUpdate(ID peer, const Update &update, spk::Message::RequestID request) override
		{
			attempts.push_back(peer);
			if (throwing)
			{
				throw spk::Exception("Send failed");
			}
			if (blocked.contains(peer))
			{
				return false;
			}
			sent.emplace_back(update, request);
			return true;
		}

	public:
		using PublicationDispatchTrait::_closeQueue;
		using PublicationDispatchTrait::_dispatchPublication;
		using PublicationDispatchTrait::_openQueue;
		using PublicationDispatchTrait::_queue;
		using PublicationDispatchTrait::PublicationDispatchTrait;
		int captures = 0;
		bool throwing = false;
		std::set<ID> blocked;
		std::vector<ID> attempts;
		std::vector<std::pair<Update, spk::Message::RequestID>> sent;
	};
};

TEST_F(PublicationDispatchTraitTest, QueueWorksWithoutClockOrSenderAndPreservesCorrelation)
{
	Queue queue(2);
	queue._openQueue(peer);
	auto first = update(1);
	auto second = update(2);
	second.object = ID::generate();
	queue._queue(peer, first, 7);
	queue._queue(peer, second, 11);
	queue._queue(peer, update(3, 2));
	ASSERT_NE(queue._front(peer), nullptr);
	EXPECT_EQ(queue._front(peer)->requestID, 7u);
	EXPECT_EQ(queue._front(peer)->update.payload->reader().get<int>(), 3);
	queue._pop(peer);
	EXPECT_EQ(queue._front(peer)->update.object, second.object);
	second.edit = Edit::Forget;
	second.payload.reset();
	queue._queue(peer, second);
	EXPECT_EQ(queue._front(peer)->requestID, 0u);
	queue._pop(peer);
	EXPECT_EQ(queue._front(peer), nullptr);
}

TEST_F(PublicationDispatchTraitTest, QueueCapacityAndDuplicateOpeningPreservePendingState)
{
	Queue queue(1);
	queue._openQueue(peer);
	queue._queue(peer, update(1));
	EXPECT_THROW(queue._openQueue(peer), spk::Exception);
	auto other = update(2);
	other.object = ID::generate();
	EXPECT_THROW(queue._queue(peer, other), spk::Exception);
	EXPECT_EQ(queue._front(peer)->update.object, object);
	queue._closeQueue(peer);
	queue._openQueue(peer);
	EXPECT_EQ(queue._front(peer), nullptr);
	EXPECT_THROW(queue._pop(peer), spk::Exception);
	queue._closeQueue(peer);
	EXPECT_THROW((void)queue._front(peer), spk::Exception);
	EXPECT_THROW(queue._pop(peer), spk::Exception);
}

TEST_F(PublicationDispatchTraitTest, CapturesOnlyOnDuePassesWithPeersAndAnAttemptBudget)
{
	Dispatcher dispatch(2, 50ms);
	EXPECT_EQ(dispatch._dispatchPublication(now, 1).sent, 0u);
	EXPECT_EQ(dispatch.captures, 0);
	dispatch._openQueue(peer);
	dispatch._queue(peer, update(1));
	EXPECT_EQ(dispatch._dispatchPublication(now, 0).sent, 0u);
	EXPECT_EQ(dispatch.captures, 0);
	EXPECT_EQ(dispatch._dispatchPublication(now, 1).sent, 1u);
	dispatch._queue(peer, update(2));
	EXPECT_EQ(dispatch._dispatchPublication(now + 49ms, 1).sent, 0u);
	EXPECT_EQ(dispatch.captures, 1);
	EXPECT_EQ(dispatch._dispatchPublication(now + 50ms, 1).sent, 1u);
	EXPECT_EQ(dispatch.captures, 2);
}

TEST_F(PublicationDispatchTraitTest, ThrowingAndBlockedSendsRetainStateAndRotateAcrossCalls)
{
	Dispatcher dispatch(2, 0ms);
	const auto other = ID::generate();
	dispatch._openQueue(peer);
	dispatch._openQueue(other);
	dispatch._queue(peer, update(1), 9);
	dispatch._queue(other, update(2));
	dispatch.throwing = true;
	EXPECT_EQ(dispatch._dispatchPublication(now, 1).errors, 1u);
	dispatch.throwing = false;
	dispatch.blocked.insert(peer);
	EXPECT_EQ(dispatch._dispatchPublication(now, 1).sent, 1u);
	EXPECT_EQ(dispatch.attempts.back(), other);
	EXPECT_EQ(dispatch._dispatchPublication(now, 1).blocked, 1u);
	dispatch._closeQueue(other);
	dispatch.blocked.clear();
	EXPECT_EQ(dispatch._dispatchPublication(now, 1).sent, 1u);
	EXPECT_EQ(dispatch.sent.back().second, 9u);
	EXPECT_EQ(dispatch.sent.back().first.payload->reader().get<int>(), 1);
}
