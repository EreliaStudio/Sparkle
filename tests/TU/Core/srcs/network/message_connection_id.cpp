#include "network/message.hpp"

#include <gtest/gtest.h>
#include <utility>

TEST(NetworkMessage, ConnectionIDIsPreservedByWriter)
{
	spk::Message::Writer writer(42);
	writer.setRequestID(17);
	writer.setConnectionID(81);
	const spk::Message message = std::move(writer).build();

	EXPECT_EQ(message.type(), 42u);
	EXPECT_EQ(message.requestID(), 17u);
	EXPECT_EQ(message.connectionID(), 81u);
	EXPECT_EQ(message.header().connectionID, 81u);
}

TEST(NetworkMessage, ConnectionIDSurvivesEditing)
{
	spk::Message::Writer writer(7);
	writer.setConnectionID(124);
	auto message = std::move(writer).build();
	spk::Message::Writer editor(std::move(message));
	editor.append(std::uint32_t{99});
	const auto edited = std::move(editor).build();

	EXPECT_EQ(edited.connectionID(), 124u);
	EXPECT_EQ(edited.size(), sizeof(std::uint32_t));
}
