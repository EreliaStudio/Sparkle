#include <gtest/gtest.h>

#include <sstream>

#include "input/input_state.hpp"

TEST(InputStateTest, StandardUsage)
{
	EXPECT_EQ(spk::toString(spk::InputState::Down), "Down");
	EXPECT_EQ(spk::toString(spk::InputState::Up), "Up");
	EXPECT_EQ(spk::toWstring(spk::InputState::Down), L"Down");
	EXPECT_EQ(spk::toWstring(spk::InputState::Up), L"Up");
}

TEST(InputStateTest, StreamsUseTheSameTokens)
{
	std::ostringstream stream;
	stream << spk::InputState::Down << ' ' << spk::InputState::Up;
	EXPECT_EQ(stream.str(), "Down Up");

	std::wostringstream wideStream;
	wideStream << spk::InputState::Down << L' ' << spk::InputState::Up;
	EXPECT_EQ(wideStream.str(), L"Down Up");
}

TEST(InputStateTest, InvalidValueProducesUnknownToken)
{
	const auto invalid = static_cast<spk::InputState>(42);
	EXPECT_EQ(spk::toString(invalid), "Unknow InputState");
	EXPECT_EQ(spk::toWstring(invalid), L"Unknow InputState");

	std::ostringstream stream;
	stream << invalid;
	EXPECT_EQ(stream.str(), "Unknow InputState");

	std::wostringstream wideStream;
	wideStream << invalid;
	EXPECT_EQ(wideStream.str(), L"Unknow InputState");
}
