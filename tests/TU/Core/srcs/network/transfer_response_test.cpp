#include "network/internal/transfer_response.hpp"

#include "exception.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

namespace
{
	using spk::NetworkInternal::TransferResponse;
	using spk::Transfer;
}

TEST(TransferResponse, EncodesAndDecodesSuccessAndFailure)
{
	for (const auto response : {
		TransferResponse{42, 17, Transfer::Status::Success, Transfer::Reason::None},
		TransferResponse{42, 18, Transfer::Status::Rejected, Transfer::Reason::ReceiverCapacity},
		TransferResponse{42, 19, Transfer::Status::Failed, Transfer::Reason::ConflictingPage}})
	{
		const auto frame = response.encode();
		ASSERT_EQ(frame->size(), spk::NetworkInternal::FrameHeaderSize + TransferResponse::PayloadSize);
		const auto decoded = TransferResponse::decode(*frame);
		EXPECT_EQ(decoded.connectionID, response.connectionID);
		EXPECT_EQ(decoded.transferID, response.transferID);
		EXPECT_EQ(decoded.status, response.status);
		EXPECT_EQ(decoded.reason, response.reason);
	}
}

TEST(TransferResponse, RejectsUnacceptableControlOutcomes)
{
	const TransferResponse indeterminate{42, 1, Transfer::Status::Indeterminate,
		Transfer::Reason::AcknowledgmentTimeout};
	const TransferResponse missingID{42, 0, Transfer::Status::Success, Transfer::Reason::None};
	const TransferResponse inconsistent{42, 1, Transfer::Status::Success, Transfer::Reason::ReceiverCapacity};
	EXPECT_THROW((void)indeterminate.encode(), spk::Exception);
	EXPECT_THROW((void)missingID.encode(), spk::Exception);
	EXPECT_THROW((void)inconsistent.encode(), spk::Exception);
}

TEST(TransferResponse, RejectsMalformedFrames)
{
	auto frame = TransferResponse{42, 1, Transfer::Status::Success, Transfer::Reason::None}.encode();
	(*frame)[spk::NetworkInternal::FrameHeaderSize + 12] = std::byte{1};
	EXPECT_THROW((void)TransferResponse::decode(*frame), spk::Exception);
	(*frame)[spk::NetworkInternal::FrameHeaderSize + 12] = std::byte{0};
	spk::NetworkInternal::write16(frame->data() + spk::NetworkInternal::FrameHeaderSize + 8, 99);
	EXPECT_THROW((void)TransferResponse::decode(*frame), spk::Exception);
	spk::NetworkInternal::write16(frame->data() + spk::NetworkInternal::FrameHeaderSize + 8,
		static_cast<std::uint16_t>(Transfer::Status::Success));
	spk::NetworkInternal::write16(frame->data() + 6, 1);
	EXPECT_THROW((void)TransferResponse::decode(*frame), spk::Exception);
}
