#include <gtest/gtest.h>

#include <string>

#include "diagnostics/logger.hpp"

TEST(LoggerTest, EntrySubscriptionReceivesLevelAndRawMessage)
{
	spk::Logger::Level receivedLevel = spk::Logger::Level::Trace;
	std::string receivedMessage;

	auto contract = spk::logger.subscribeToEntry(
		[&](const spk::Logger::Level &level, const std::string &message) {
			receivedLevel = level;
			receivedMessage = message;
		});

	SPK_LOG(Warning) << "raw logger entry" << std::endl;

	EXPECT_EQ(receivedLevel, spk::Logger::Level::Warning);
	EXPECT_EQ(receivedMessage, "raw logger entry");
}

TEST(LoggerTest, EntrySubscriptionDoesNotReceiveSourceLocation)
{
	std::string receivedMessage;
	auto contract = spk::logger.subscribeToEntry(
		[&](const spk::Logger::Level &, const std::string &message) {
			receivedMessage = message;
		});

	SPK_LOG(Info) << "message without source metadata" << std::endl;

	EXPECT_EQ(receivedMessage, "message without source metadata");
	EXPECT_EQ(receivedMessage.find(__FILE__), std::string::npos);
}

TEST(LoggerTest, ResignedEntrySubscriptionStopsReceivingEntries)
{
	std::size_t callCount = 0;
	auto contract = spk::logger.subscribeToEntry(
		[&](const spk::Logger::Level &, const std::string &) {
			++callCount;
		});

	SPK_LOG(Info) << "first" << std::endl;
	contract.resign();
	SPK_LOG(Info) << "second" << std::endl;

	EXPECT_EQ(callCount, 1u);
}
