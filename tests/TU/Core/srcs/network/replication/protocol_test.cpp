#include "network/network_traits_test.hpp"

TEST_F(NetworkTraitsTest, ProtocolDecodesStateOnceAndRoundTripsRequestCorrelation)
{
	const auto frame = protocol.encode(update(5), 17);
	EXPECT_EQ(protocol.kind(frame), Protocol::Kind::Update);
	EXPECT_EQ(frame.requestID(), 17u);
	Codec::decodes = 0;
	const auto decoded = protocol.decodeUpdate(frame);
	EXPECT_EQ(decoded.state->value, 5);
	EXPECT_EQ(Codec::decodes, 1);
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
	invalid.state.reset();
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
