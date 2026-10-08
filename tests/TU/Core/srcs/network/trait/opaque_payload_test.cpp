#include "network/network_traits_test.hpp"

TEST_F(NetworkTraitsTest, SnapshotSerializesOnceAndSharesImmutableBytesAcrossCopies)
{
	Object entity;
	entity.change(7);
	auto snapshot = entity.captureNetworkState();
	const auto shared = snapshot;
	entity.change(8);
	EXPECT_EQ(entity.builds, 1);
	EXPECT_EQ(shared.reader().get<int>(), 7);
	EXPECT_EQ(shared.data().data(), snapshot.data().data());
	spk::Message::Writer edited(std::move(snapshot));
	edited.edit<int>(0, 99);
	EXPECT_EQ(shared.reader().get<int>(), 7);
	EXPECT_EQ(std::move(edited).build().reader().get<int>(), 99);
}

TEST_F(NetworkTraitsTest, OneChannelCanPublishDifferentApplicationFormatsToMultiplePeers)
{
	class Text final : public spk::Network::PublishableTrait, public spk::Network::ReplicableTrait
	{
		void _writeNetworkState(spk::Message::Writer &writer) const override
		{
			++writes;
			writer << value;
		}
		void _readNetworkState(const spk::Message::Reader &reader) override
		{
			reader >> value;
		}

	public:
		std::string value;
		mutable int writes = 0;
	} text;
	class Collection final : public spk::Network::ReplicaCollectionTrait
	{
		spk::Network::ReplicableTrait *_findReplica(ID id) override
		{
			return live.contains(id) ? &_object(id) : nullptr;
		}
		spk::Network::ReplicableTrait &_createReplica(ID id) override
		{
			auto &object = _object(id);
			live.insert(id);
			return object;
		}
		void _removeReplica(ID id) override
		{
			live.erase(id);
		}
		spk::Network::ReplicableTrait &_object(ID id)
		{
			if (id == numberID)
			{
				return number;
			}
			if (id == textID)
			{
				return text;
			}
			throw spk::Exception("Unknown application object");
		}

	public:
		Collection(ID numberID, ID textID) :
			ReplicaCollectionTrait(42),
			numberID(numberID),
			textID(textID)
		{
		}
		ID numberID, textID;
		Object number;
		Text text;
		std::set<ID> live;
	};
	Source source;
	Object number;
	number.change(7);
	text.value = "different layout";
	const auto textID = ID::generate(), otherPeer = ID::generate();
	Collection first(object, textID), second(object, textID);
	first.resetSession(source.openPeer(peer));
	second.resetSession(source.openPeer(otherPeer));
	source.registerObject(object, number);
	source.registerObject(textID, text);
	for (auto recipient : {peer, otherPeer})
	{
		source.follow(recipient, object);
		source.follow(recipient, textID);
	}
	ASSERT_EQ(source.dispatch(now).sent, 4u);
	EXPECT_EQ(number.builds, 1);
	EXPECT_EQ(text.writes, 1);
	for (const auto &[recipient, message] : source.sent)
	{
		EXPECT_TRUE((recipient == peer ? first : second).receiveMessage(message));
	}
	EXPECT_EQ(first.number.value, 7);
	EXPECT_EQ(second.number.value, 7);
	EXPECT_EQ(first.text.value, "different layout");
	EXPECT_EQ(second.text.value, "different layout");
}

TEST_F(NetworkTraitsTest, EmptyObjectFormatCanBeCapturedAndApplied)
{
	class Empty final : public spk::Network::PublishableTrait, public spk::Network::ReplicableTrait
	{
		void _writeNetworkState(spk::Message::Writer &) const override
		{
		}
		void _readNetworkState(const spk::Message::Reader &) override
		{
			++reads;
		}

	public:
		int reads = 0;
	} object;
	auto snapshot = object.captureNetworkState();
	EXPECT_TRUE(snapshot.empty());
	EXPECT_NO_THROW(object.readNetworkState(snapshot.reader()));
	EXPECT_EQ(object.reads, 1);
}
