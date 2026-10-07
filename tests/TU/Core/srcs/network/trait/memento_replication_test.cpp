#include "design_pattern/trait/memento_trait.hpp"
#include "network/network.hpp"
#include <gtest/gtest.h>
#include <map>

class NetworkMementoTest : public testing::Test
{
protected:
	using ID = spk::UUID;
	class Object : public spk::Network::ReplicableTrait, public spk::MementoTrait
	{

	protected:
		void _readNetworkState(const spk::Message::Reader &reader) override
		{
			reader >> number; // Deliberately mutates before parsing the next field.
			reader >> name;
		}
		Snapshot _save() const override
		{
			auto snapshot = Snapshot::object();
			snapshot["number"] = number;
			snapshot["name"] = name;
			return snapshot;
		}
		void _restore(const Snapshot &snapshot) override
		{
			const auto restoredNumber = snapshot.at("number").as<int>();
			auto restoredName = snapshot.at("name").as<std::string>();
			name = std::move(restoredName);
			number = restoredNumber;
			++restorations;
		}

	public:
		int number = 1, restorations = 0;
		std::string name = "original";
		void readSafely(const spk::Message::Reader &reader)
		{
			const auto previous = save();
			try
			{
				readNetworkState(reader);
			} catch (...)
			{
				restore(previous);
				throw;
			}
		}
	};
	class Safe final : public Object
	{
		void _readNetworkState(const spk::Message::Reader &reader) override
		{
			const auto previous = save();
			try
			{
				Object::_readNetworkState(reader);
				// Include all application validation in the explicit rollback boundary.
				if (reader.readOffset() != reader.size())
				{
					throw spk::Exception("Trailing state");
				}
			} catch (...)
			{
				restore(previous);
				throw;
			}
		}
	};
	class Collection final : public spk::Network::ReplicaCollectionTrait
	{
		spk::Network::ReplicableTrait *_findReplica(ID id) override
		{
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
		bool _sendMessage(const spk::Message &message) override
		{
			requestID = message.requestID();
			return true;
		}

	public:
		Collection() :
			ReplicaCollectionTrait(42)
		{
		}
		std::map<ID, Object> objects;
		spk::Message::RequestID requestID = 0;
	};
	static spk::Message incomplete(int number)
	{
		spk::Message::Writer writer;
		writer << number;
		return std::move(writer).build();
	}
	static spk::Message complete(int number, const std::string &name, bool trailing = false)
	{
		spk::Message::Writer writer;
		writer << number << name;
		if (trailing)
		{
			writer << 99;
		}
		return std::move(writer).build();
	}
	ID session = ID::generate(), object = ID::generate();
	spk::Network::Protocol protocol{42};
	spk::Message update(spk::Message payload, std::uint64_t revision, spk::Message::RequestID requestID = 0)
	{
		return protocol.encode({session, object, 1, revision, spk::Network::Edit::Set, std::move(payload)}, requestID);
	}
};

TEST_F(NetworkMementoTest, InheritingMementoDoesNotAutomaticallyRollBackAFailedRead)
{
	Object value;
	EXPECT_THROW(value.readNetworkState(incomplete(9).reader()), spk::Exception);
	EXPECT_EQ(value.number, 9);
	EXPECT_EQ(value.name, "original");
	EXPECT_EQ(value.restorations, 0);
	EXPECT_NO_THROW(value.readNetworkState(complete(12, "recovered").reader()));
	EXPECT_EQ(value.number, 12);
	EXPECT_EQ(value.name, "recovered");
}

TEST_F(NetworkMementoTest, ApplicationTryCatchRestoresAllFieldsAndPropagatesTheReadFailure)
{
	Object value;
	EXPECT_THROW(value.readSafely(incomplete(9).reader()), spk::Exception);
	EXPECT_EQ(value.number, 1);
	EXPECT_EQ(value.name, "original");
	EXPECT_EQ(value.restorations, 1);
	EXPECT_NO_THROW(value.readSafely(complete(12, "accepted").reader()));
	EXPECT_EQ(value.number, 12);
	EXPECT_EQ(value.name, "accepted");
	EXPECT_EQ(value.restorations, 1);
}

TEST_F(NetworkMementoTest, ExplicitRollbackAlsoCoversTrailingBytesAfterSuccessfulFieldParsing)
{
	Object value;
	EXPECT_THROW(value.readSafely(complete(9, "partial", true).reader()), spk::Exception);
	EXPECT_EQ(value.number, 1);
	EXPECT_EQ(value.name, "original");
	EXPECT_EQ(value.restorations, 1);
}

TEST_F(NetworkMementoTest, PipelineLeavesExistingReplicaPartialAndDoesNotCompleteOrCommitFailedUpdate)
{
	Collection replicas;
	replicas.resetSession(session);
	ASSERT_TRUE(replicas.receiveMessage(update(complete(1, "original"), 1)));
	ASSERT_TRUE(replicas.requestObject(object));
	EXPECT_THROW((void)replicas.receiveMessage(update(incomplete(9), 2, replicas.requestID)), spk::Exception);
	EXPECT_EQ(replicas.objects.at(object).number, 9);
	EXPECT_EQ(replicas.objects.at(object).restorations, 0);
	EXPECT_EQ(replicas.requestStatus(object), Collection::RequestStatus::Pending);
	ASSERT_TRUE(replicas.receiveMessage(update(complete(10, "retry"), 2, replicas.requestID)));
	EXPECT_EQ(replicas.objects.at(object).name, "retry");
	EXPECT_EQ(replicas.requestStatus(object), Collection::RequestStatus::Ready);
}

TEST_F(NetworkMementoTest, CallerCanRollBackAroundCollectionReceiveAndRetryTheSameRevision)
{
	Collection replicas;
	replicas.resetSession(session);
	ASSERT_TRUE(replicas.receiveMessage(update(complete(1, "original"), 1)));
	auto &value = replicas.objects.at(object);
	const auto previous = value.save();
	auto receiveSafely = [&](const spk::Message &message) {
		try
		{
			return replicas.receiveMessage(message);
		} catch (...)
		{
			value.restore(previous);
			throw;
		}
	};
	EXPECT_THROW((void)receiveSafely(update(incomplete(9), 2)), spk::Exception);
	EXPECT_EQ(value.number, 1);
	EXPECT_EQ(value.name, "original");
	EXPECT_EQ(value.restorations, 1);
	EXPECT_TRUE(receiveSafely(update(complete(10, "retry"), 2)));
	EXPECT_EQ(value.number, 10);
	EXPECT_EQ(value.name, "retry");
}

TEST_F(NetworkMementoTest, FailedInitialParsingStillRemovesTheNewReplica)
{
	Collection replicas;
	replicas.resetSession(session);
	EXPECT_THROW((void)replicas.receiveMessage(update(incomplete(9), 1)), spk::Exception);
	EXPECT_TRUE(replicas.objects.empty());
	EXPECT_TRUE(replicas.receiveMessage(update(complete(10, "retry"), 1)));
	EXPECT_EQ(replicas.objects.at(object).number, 10);
}

TEST_F(NetworkMementoTest, ObjectCanOptIntoRollbackInsideItsReadHookWithoutPipelineChanges)
{
	class Collection final : public spk::Network::ReplicaCollectionTrait
	{
		spk::Network::ReplicableTrait *_findReplica(ID) override
		{
			return live ? &value : nullptr;
		}
		spk::Network::ReplicableTrait &_createReplica(ID) override
		{
			live = true;
			return value;
		}
		void _removeReplica(ID) override
		{
			live = false;
		}

	public:
		Collection() :
			ReplicaCollectionTrait(42)
		{
		}
		Safe value;
		bool live = false;
	} replicas;
	replicas.resetSession(session);
	ASSERT_TRUE(replicas.receiveMessage(update(complete(1, "original"), 1)));
	EXPECT_THROW((void)replicas.receiveMessage(update(incomplete(9), 2)), spk::Exception);
	EXPECT_EQ(replicas.value.number, 1);
	EXPECT_EQ(replicas.value.name, "original");
	EXPECT_THROW((void)replicas.receiveMessage(update(complete(9, "partial", true), 2)), spk::Exception);
	EXPECT_EQ(replicas.value.number, 1);
	EXPECT_EQ(replicas.value.name, "original");
	EXPECT_EQ(replicas.value.restorations, 2);
	EXPECT_TRUE(replicas.receiveMessage(update(complete(10, "retry"), 2)));
	EXPECT_EQ(replicas.value.number, 10);
	EXPECT_EQ(replicas.value.name, "retry");
}
