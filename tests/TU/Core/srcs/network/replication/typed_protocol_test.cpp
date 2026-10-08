#include "network/network_traits_test.hpp"
#include <algorithm>

namespace
{
	struct State
	{
		int number = 0;
		std::string name;
		State() = default;
		State(const State &) = delete;
		State(State &&) = default;
		State &operator=(State &&) = default;
	};
	spk::Message::Writer &operator<<(spk::Message::Writer &writer, const State &state)
	{
		return writer << state.number << state.name;
	}
	const spk::Message::Reader &operator>>(const spk::Message::Reader &reader, State &state)
	{
		return reader >> state.number >> state.name;
	}
	struct WriteOnly
	{
		std::string text;
		explicit WriteOnly(std::string text) :
			text(std::move(text))
		{
		}
	};
	spk::Message::Writer &operator<<(spk::Message::Writer &writer, const WriteOnly &state)
	{
		return writer << state.text;
	}
	struct ReadOnly
	{
		std::string text;
	};
	const spk::Message::Reader &operator>>(const spk::Message::Reader &reader, ReadOnly &state)
	{
		return reader >> state.text;
	}
	template <typename T>
	concept Encodable = requires(const spk::Network::Protocol &protocol, const spk::Network::Update<T> &update) { protocol.encode(update); };
	template <typename T>
	concept Decodable = requires(const spk::Network::Protocol &protocol, const spk::Message &message) { protocol.template decodeUpdate<T>(message); };
	static_assert(spk::MessageSerializable<State>);
	static_assert(Encodable<WriteOnly> && !Decodable<WriteOnly>);
	static_assert(Decodable<ReadOnly> && !Encodable<ReadOnly>);
}

TEST_F(NetworkTraitsTest, TypedProtocolRoundTripsMoveOnlyStateAndMetadata)
{
	spk::Network::Update<State> value{session, object, 3, 7, Edit::Set, State{}};
	value.payload->number = 12;
	value.payload->name = "state";
	const auto frame = protocol.encode(value, 29);
	const auto decoded = protocol.decodeUpdate<State>(frame);
	ASSERT_TRUE(decoded.payload);
	EXPECT_EQ(decoded.payload->number, 12);
	EXPECT_EQ(decoded.payload->name, "state");
	EXPECT_EQ(decoded.session, session);
	EXPECT_EQ(decoded.object, object);
	EXPECT_EQ(decoded.tracking, 3u);
	EXPECT_EQ(decoded.revision, 7u);
	EXPECT_EQ(frame.requestID(), 29u);
}

TEST_F(NetworkTraitsTest, TypedProtocolSupportsIndependentReadAndWriteContracts)
{
	spk::Network::Update<WriteOnly> value{session, object, 1, 1, Edit::Set, WriteOnly{"hello"}};
	const auto decoded = protocol.decodeUpdate<ReadOnly>(protocol.encode(value));
	ASSERT_TRUE(decoded.payload);
	EXPECT_EQ(decoded.payload->text, "hello");
}

TEST_F(NetworkTraitsTest, TypedRemovalNeedsNoStateAndInvalidPresenceIsRejected)
{
	for (const auto edit : {Edit::Forget, Edit::Destroy})
	{
		spk::Network::Update<State> value{session, object, 1, 1, edit, std::nullopt};
		const auto decoded = protocol.decodeUpdate<State>(protocol.encode(value));
		EXPECT_EQ(decoded.edit, edit);
		EXPECT_FALSE(decoded.payload);
		value.payload.emplace();
		EXPECT_THROW((void)protocol.encode(value), spk::Exception);
	}
	spk::Network::Update<State> missing{session, object, 1, 1, Edit::Set, std::nullopt};
	EXPECT_THROW((void)protocol.encode(missing), spk::Exception);
}

TEST_F(NetworkTraitsTest, TypedDecodeRejectsPartialFieldsAndTrailingApplicationData)
{
	EXPECT_THROW((void)protocol.decodeUpdate<State>(protocol.encode(update(5))), spk::Exception);
	spk::Message::Writer writer;
	writer << 5 << std::string("ok") << 99;
	auto value = update(5);
	value.payload = std::move(writer).build();
	EXPECT_THROW((void)protocol.decodeUpdate<State>(protocol.encode(value)), spk::Exception);
	EXPECT_EQ(protocol.decodeUpdate(protocol.encode(value)).payload->size(), value.payload->size());
}

TEST_F(NetworkTraitsTest, TypedProtocolKeepsFrameLimitsAndEnvelopeValidation)
{
	spk::Network::Update<std::string> value{session, object, 1, 1, Edit::Set, std::string(1024, 'a')};
	EXPECT_THROW((void)Protocol(42, 64).encode(value), spk::Exception);
	value.object = {};
	EXPECT_THROW((void)protocol.encode(value), spk::Exception);
	const auto frame = protocol.encode(update(7));
	for (std::size_t length = 0; length < frame.size(); ++length)
	{
		spk::Message::Writer writer(42);
		writer.append(frame.data().data(), length);
		EXPECT_THROW((void)protocol.decodeUpdate<int>(std::move(writer).build()), spk::Exception);
	}
}

TEST_F(NetworkTraitsTest, TypedAndCapturedObjectPayloadsUseTheSameWireFormat)
{
	spk::Network::Update<int> value{session, object, 1, 1, Edit::Set, 42};
	const auto typed = protocol.encode(value);
	const auto captured = protocol.encode(update(42));
	EXPECT_TRUE(std::ranges::equal(typed.data(), captured.data()));
	Object replica;
	replica.readNetworkState(protocol.decodeUpdate(typed).payload->reader());
	EXPECT_EQ(replica.value, 42);
	EXPECT_EQ(protocol.decodeUpdate<int>(captured).payload, 42);
	Replicas collection;
	collection.resetSession(session);
	ASSERT_TRUE(collection.receiveMessage(typed));
	EXPECT_EQ(collection.objects.at(object).value, 42);
}
