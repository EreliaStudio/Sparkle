#include "network/network_traits_test.hpp"

class PublicationRequestTraitTest : public NetworkTraitsTest
{
protected:
	class Publisher : public spk::Network::PublicationRequestTrait
	{
		bool _sendUpdate(ID, const Update &update, spk::Message::RequestID request) override
		{
			updates.emplace_back(update, request);
			return true;
		}
		bool _sendRejection(ID, const Request &request) override
		{
			if (onReject)
			{
				onReject();
			}
			if (blocked)
			{
				return false;
			}
			rejections.push_back(request);
			return true;
		}
		void _requestObject(ID peer, const Request &request) override
		{
			if (!deferred)
			{
				PublicationRequestTrait::_requestObject(peer, request);
			}
		}

	public:
		Publisher() :
			PublicationRequestTrait({.interval = Clock::duration::zero()})
		{
		}
		bool blocked = false, deferred = false;
		std::function<void()> onReject;
		std::vector<std::pair<Update, spk::Message::RequestID>> updates;
		std::vector<Request> rejections;
	};
};

TEST_F(PublicationRequestTraitTest, ServesDecodedRequestsWithoutProtocolOrServer)
{
	Publisher source;
	Object entity;
	entity.change(12);
	source.registerObject(object, entity);
	const Request request{source.openPeer(peer), object, 1};
	ASSERT_TRUE(source.receiveRequest(peer, request));
	EXPECT_EQ(source.dispatch(now).sent, 1u);
	ASSERT_EQ(source.updates.size(), 1u);
	EXPECT_EQ(source.updates.front().first.payload->reader().get<int>(), 12);
	EXPECT_EQ(source.updates.front().second, request.id);
	EXPECT_FALSE(source.acceptRequest(peer, request));
	source.forget(peer, object);
	EXPECT_EQ(source.dispatch(now).sent, 1u);
	EXPECT_EQ(source.updates.back().first.edit, Edit::Forget);
}

TEST_F(PublicationRequestTraitTest, FailedRejectionRemainsRetryableAndSharesPublicationGuard)
{
	Publisher source;
	const Request request{source.openPeer(peer), object, 1};
	source.blocked = true;
	ASSERT_TRUE(source.receiveRequest(peer, request));
	EXPECT_TRUE(source.rejections.empty());
	source.blocked = false;
	source.onReject = [&] {
		source.closePeer(peer);
	};
	EXPECT_THROW((void)source.rejectRequest(peer, request), spk::Exception);
	source.onReject = {};
	EXPECT_TRUE(source.rejectRequest(peer, request));
	EXPECT_EQ(source.rejections.size(), 1u);
	EXPECT_FALSE(source.rejectRequest(peer, request));
}

TEST_F(PublicationRequestTraitTest, SupersededAndClosedRequestsCannotPublishDeferredPayloads)
{
	Publisher source;
	source.deferred = true;
	const auto session = source.openPeer(peer);
	const Request first{session, object, 1}, second{session, object, 2};
	ASSERT_TRUE(source.receiveRequest(peer, first));
	ASSERT_TRUE(source.receiveRequest(peer, second));
	EXPECT_FALSE(source.fulfillRequest(peer, first, payload(1)));
	EXPECT_TRUE(source.fulfillRequest(peer, second, payload(2)));
	EXPECT_EQ(source.dispatch(now).sent, 1u);
	EXPECT_EQ(source.updates.back().first.payload->reader().get<int>(), 2);
	const Request third{session, object, 3};
	ASSERT_TRUE(source.receiveRequest(peer, third));
	source.closePeer(peer);
	EXPECT_FALSE(source.fulfillRequest(peer, third, payload(3)));
}
