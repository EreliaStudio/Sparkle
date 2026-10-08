#include "network/network_traits_test.hpp"

class RequestedReplicaTraitTest : public NetworkTraitsTest
{
protected:
	class Collection : public spk::Network::RequestedReplicaTrait
	{
		bool _sendObjectRequest(const Request &request) override
		{
			requests.push_back(request);
			return true;
		}
		spk::Network::ReplicableTrait *_findReplica(ID id) override
		{
			if (onApply)
			{
				onApply();
			}
			if (failApply)
			{
				throw spk::Exception("Application failure");
			}
			auto found = objects.find(id);
			return found == objects.end() ? nullptr : &found->second;
		}
		spk::Network::ReplicableTrait &_createReplica(ID id) override
		{
			return objects[id];
		}
		void _removeReplica(ID id) override
		{
			objects.erase(id);
		}

	public:
		std::map<ID, Object> objects;
		std::vector<Request> requests;
		std::function<void()> onApply;
		bool failApply = false;
	};
};

TEST_F(RequestedReplicaTraitTest, MatchingDuplicateCompletesAcquisitionWithoutReapplyingState)
{
	Collection replicas;
	replicas.resetSession(session);
	ASSERT_TRUE(replicas.receiveUpdate(update(7)));
	ASSERT_TRUE(replicas.requestObject(object));
	const auto request = replicas.requests.back();
	ASSERT_TRUE(replicas.receiveUpdate(update(7), request.id));
	EXPECT_EQ(replicas.requestStatus(object), Status::Ready);
	EXPECT_EQ(replicas.objects.at(object).applications, 1);
}

TEST_F(RequestedReplicaTraitTest, OldResponseAppliesStateButCannotCompleteNewAcquisition)
{
	Collection replicas;
	replicas.resetSession(session);
	ASSERT_TRUE(replicas.requestObject(object));
	const auto old = replicas.requests.back();
	ASSERT_TRUE(replicas.requestObject(object));
	const auto current = replicas.requests.back();
	EXPECT_TRUE(replicas.receiveUpdate(update(8), old.id));
	EXPECT_EQ(replicas.requestStatus(object), Status::Pending);
	EXPECT_FALSE(replicas.receiveRejection(old));
	EXPECT_TRUE(replicas.receiveUpdate(update(8), current.id));
	EXPECT_EQ(replicas.requestStatus(object), Status::Ready);
	EXPECT_EQ(replicas.objects.at(object).applications, 1);
}

TEST_F(RequestedReplicaTraitTest, FailedApplicationAndReentryLeaveAcquisitionRetryable)
{
	Collection replicas;
	replicas.resetSession(session);
	ASSERT_TRUE(replicas.requestObject(object));
	const auto request = replicas.requests.back();
	replicas.failApply = true;
	EXPECT_THROW((void)replicas.receiveUpdate(update(9), request.id), spk::Exception);
	EXPECT_EQ(replicas.requestStatus(object), Status::Pending);
	replicas.failApply = false;
	replicas.onApply = [&] {
		spk::Network::ObjectRequesterTrait &requester = replicas;
		requester.cancelRequest(object);
	};
	EXPECT_THROW((void)replicas.receiveUpdate(update(9), request.id), spk::Exception);
	EXPECT_EQ(replicas.requestStatus(object), Status::Pending);
	replicas.onApply = {};
	EXPECT_TRUE(replicas.receiveUpdate(update(9), request.id));
	EXPECT_EQ(replicas.requestStatus(object), Status::Ready);
}

TEST_F(RequestedReplicaTraitTest, RemovalAndSessionChangesClearAcquisitions)
{
	Collection replicas;
	replicas.resetSession(session);
	ASSERT_TRUE(replicas.requestObject(object));
	const auto request = replicas.requests.back();
	ASSERT_TRUE(replicas.receiveUpdate(update(10), request.id));
	auto removal = update(10);
	removal.edit = Edit::Forget;
	removal.payload.reset();
	EXPECT_TRUE(replicas.receiveUpdate(removal, 0));
	EXPECT_FALSE(replicas.requestStatus(object));
	EXPECT_TRUE(replicas.objects.empty());
	ASSERT_TRUE(replicas.requestObject(object));
	replicas.resetSession(ID::generate());
	EXPECT_FALSE(replicas.requestStatus(object));
	EXPECT_FALSE(replicas.receiveUpdate(update(11), request.id));
	EXPECT_FALSE(replicas.receiveRejection(request));
}
