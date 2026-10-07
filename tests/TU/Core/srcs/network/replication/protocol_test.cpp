#include "network/network_traits_test.hpp"
#include <limits>

TEST_F(NetworkTraitsTest, ProtocolPreservesOpaquePayloadAndRequestCorrelation)
{
	const auto frame = protocol.encode(update(5), 17);
	EXPECT_EQ(protocol.kind(frame), Protocol::Kind::Update);
	EXPECT_EQ(frame.requestID(), 17u);
	Object::reads = 0;
	const auto decoded = protocol.decodeUpdate(frame);
	EXPECT_EQ(decoded.payload->reader().get<int>(), 5);
	EXPECT_EQ(Object::reads, 0);
	const Request request{session, object, 3};
	EXPECT_EQ(protocol.decodeRequest(protocol.encode(request)), request);
	EXPECT_EQ(protocol.encode(request).requestID(), 3u);
	EXPECT_EQ(protocol.decodeRequest(protocol.encode(request, Protocol::Kind::Rejected), Protocol::Kind::Rejected), request);
}

TEST_F(NetworkTraitsTest, ProtocolRejectsEveryTruncationAndTrailingDataForAllMessages)
{
	const Request request{session, object, 3};
	const std::vector<spk::Message> frames = {protocol.encode(update(5)), protocol.encode(request), protocol.encode(request, Protocol::Kind::Rejected)};
	for (const auto &frame : frames)
	{
		auto decode = [&](const spk::Message &message) {
			if (protocol.kind(frame) == Protocol::Kind::Update)
			{
				(void)protocol.decodeUpdate(message);
			}
			else
			{
				(void)protocol.decodeRequest(message, protocol.kind(frame));
			}
		};
		for (std::size_t size = 0; size < frame.size(); ++size)
		{
			spk::Message::Writer writer(42);
			writer.setRequestID(frame.requestID());
			writer.append(frame.data().data(), size);
			EXPECT_THROW(decode(std::move(writer).build()), spk::Exception);
		}
		spk::Message::Writer writer(42);
		writer.setRequestID(frame.requestID());
		writer.append(frame.data().data(), frame.size());
		writer << 1;
		EXPECT_THROW(decode(std::move(writer).build()), spk::Exception);
	}
}

TEST_F(NetworkTraitsTest, ProtocolRejectsInvalidIdentityMetadataKindAndFrameLimits)
{
	auto invalid = update(1);
	invalid.object = {};
	EXPECT_THROW((void)protocol.encode(invalid), spk::Exception);
	invalid = update(1, 0);
	EXPECT_THROW((void)protocol.encode(invalid), spk::Exception);
	invalid = update(1);
	invalid.payload.reset();
	EXPECT_THROW((void)protocol.encode(invalid), spk::Exception);
	EXPECT_THROW((void)protocol.encode(Request{session, object, 0}), spk::Exception);
	EXPECT_THROW((void)protocol.decodeRequest(protocol.encode(update(1))), spk::Exception);
	EXPECT_THROW((void)Protocol(42, 1), spk::Exception);
	auto frame = protocol.encode(update(1));
	spk::Message::Writer writer(std::move(frame));
	writer.setType(43);
	EXPECT_THROW((void)protocol.kind(std::move(writer).build()), spk::Exception);
	frame = protocol.encode(update(1));
	writer = spk::Message::Writer(std::move(frame));
	writer.edit<std::uint8_t>(sizeof(std::uint32_t), 255);
	EXPECT_THROW((void)protocol.kind(std::move(writer).build()), spk::Exception);
	frame = protocol.encode(update(1));
	writer = spk::Message::Writer(std::move(frame));
	writer.resize(65);
	EXPECT_THROW((void)Protocol(42, 64).decodeUpdate(std::move(writer).build()), spk::Exception);
}

TEST_F(NetworkTraitsTest, HandshakeRoundTripsAndRejectsTruncationTrailingDataAndInvalidMetadata)
{
	const auto token = ID::generate();
	for (const auto &frame : {protocol.encodeHandshake(token), protocol.encodeHandshake(token, session)})
	{
		const auto handshake = protocol.decodeHandshake(frame);
		EXPECT_EQ(handshake.token, token);
		EXPECT_EQ(handshake.session.isNull(), protocol.kind(frame) == Protocol::Kind::Hello);
		for (std::size_t length = 0; length < frame.size(); ++length)
		{
			spk::Message::Writer writer(42);
			writer.append(frame.data().data(), length);
			EXPECT_THROW((void)protocol.decodeHandshake(std::move(writer).build()), spk::Exception);
		}
		auto copy = frame;
		spk::Message::Writer writer(std::move(copy));
		writer << 1;
		EXPECT_THROW((void)protocol.decodeHandshake(std::move(writer).build()), spk::Exception);
		copy = frame;
		writer = spk::Message::Writer(std::move(copy));
		writer.setRequestID(1);
		EXPECT_THROW((void)protocol.decodeHandshake(std::move(writer).build()), spk::Exception);
	}
	EXPECT_THROW((void)protocol.encodeHandshake({}), spk::Exception);
	EXPECT_THROW((void)protocol.decodeHandshake(protocol.encode(update(1))), spk::Exception);
	auto frame = protocol.encodeHandshake(token);
	spk::Message::Writer writer(std::move(frame));
	writer.edit(sizeof(std::uint32_t), Protocol::Kind::Session);
	EXPECT_THROW((void)protocol.decodeHandshake(std::move(writer).build()), spk::Exception);
}

TEST_F(NetworkTraitsTest, EmptySerializedStateIsDistinctFromMissingState)
{
	auto value = update(1);
	value.payload = std::move(spk::Message::Writer{}).build();
	const auto decoded = protocol.decodeUpdate(protocol.encode(value));
	ASSERT_TRUE(decoded.payload.has_value());
	EXPECT_TRUE(decoded.payload->empty());
	value.payload.reset();
	EXPECT_THROW((void)protocol.encode(value), spk::Exception);
	value.edit = Edit::Forget;
	EXPECT_FALSE(protocol.decodeUpdate(protocol.encode(value)).payload.has_value());
}

TEST_F(NetworkTraitsTest, DeclaredPayloadLengthMustMatchRemainingBytesBeforeCopying)
{
	const auto frame = protocol.encode(update(1));
	constexpr auto lengthOffset = sizeof(std::uint32_t) + sizeof(Protocol::Kind) + 2 * sizeof(ID) + 2 * sizeof(std::uint64_t) + sizeof(Edit);
	for (const auto length : {0u, 3u, 5u, std::numeric_limits<std::uint32_t>::max()})
	{
		auto copy = frame;
		spk::Message::Writer writer(std::move(copy));
		writer.edit(lengthOffset, length);
		EXPECT_THROW((void)protocol.decodeUpdate(std::move(writer).build()), spk::Exception);
	}
	auto value = update(1);
	spk::Message::Writer payload;
	payload.resize(1024);
	value.payload = std::move(payload).build();
	EXPECT_THROW((void)Protocol(42, 64).encode(value), spk::Exception);
}

TEST_F(NetworkTraitsTest, ProtocolDoesNotParseApplicationFieldsOrAcceptOlderWireVersion)
{
	auto value = update(1);
	spk::Message::Writer opaque;
	opaque << std::uint8_t{255};
	value.payload = std::move(opaque).build();
	const auto frame = protocol.encode(value);
	const auto decoded = protocol.decodeUpdate(frame);
	ASSERT_EQ(decoded.payload->size(), 1u);
	Object replica;
	EXPECT_THROW(replica.readNetworkState(decoded.payload->reader()), spk::Exception);
	auto copy = frame;
	spk::Message::Writer old(std::move(copy));
	old.edit<std::uint32_t>(0, 0x32525053);
	EXPECT_THROW((void)protocol.decodeUpdate(std::move(old).build()), spk::Exception);
}
