#include <gtest/gtest.h>

#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "input/keyboard.hpp"

TEST(KeyboardTest, StandardUsage)
{
	spk::Keyboard keyboard;
	for (const spk::InputState state : keyboard.keys)
	{
		EXPECT_EQ(state, spk::InputState::Up);
	}
	EXPECT_EQ(keyboard.glyph, U'\0');

	keyboard[spk::Keyboard::A] = spk::InputState::Down;
	keyboard[spk::Keyboard::Escape] = spk::InputState::Down;
	keyboard.glyph = U'X';

	const spk::Keyboard &constant = keyboard;
	EXPECT_EQ(constant[spk::Keyboard::A], spk::InputState::Down);
	EXPECT_EQ(constant[spk::Keyboard::Escape], spk::InputState::Down);
	EXPECT_EQ(constant[spk::Keyboard::B], spk::InputState::Up);
	EXPECT_EQ(constant.glyph, U'X');

	EXPECT_EQ(spk::toString(spk::Keyboard::A), "A");
	EXPECT_EQ(spk::toWstring(spk::Keyboard::A), L"A");
	EXPECT_EQ(spk::fromString("A"), spk::Keyboard::A);

	std::ostringstream stream;
	stream << spk::Keyboard::A;
	EXPECT_EQ(stream.str(), "A");

	std::wostringstream wideStream;
	wideStream << spk::Keyboard::A;
	EXPECT_EQ(wideStream.str(), L"A");
}

TEST(KeyboardTest, RepresentativeKeyFamiliesRoundTrip)
{
	const std::vector<std::pair<spk::Keyboard::Key, std::string>> cases = {
		{spk::Keyboard::Escape, "Escape"},
		{spk::Keyboard::Key7, "Key7"},
		{spk::Keyboard::Q, "Q"},
		{spk::Keyboard::Numpad4, "Numpad4"},
		{spk::Keyboard::F12, "F12"},
		{spk::Keyboard::SemiColon, "SemiColon"},
		{spk::Keyboard::LeftBracket, "LeftBracket"},
		{spk::Keyboard::LeftShift, "LeftShift"},
		{spk::Keyboard::RightShift, "RightShift"},
		{spk::Keyboard::LeftControl, "LeftControl"},
		{spk::Keyboard::RightControl, "RightControl"},
		{spk::Keyboard::Unknown, "Unknown"},
	};

	for (const auto &[key, name] : cases)
	{
		SCOPED_TRACE(name);
		EXPECT_EQ(spk::toString(key), name);
		EXPECT_EQ(spk::toWstring(key), std::wstring(name.begin(), name.end()));
		ASSERT_TRUE(spk::fromString(name).has_value());
		EXPECT_EQ(*spk::fromString(name), key);
	}
}

TEST(KeyboardTest, ParsingIsCaseSensitiveAndDoesNotAcceptAliases)
{
	EXPECT_FALSE(spk::fromString("escape").has_value());
	EXPECT_FALSE(spk::fromString("ESCAPE").has_value());
	EXPECT_FALSE(spk::fromString("Esc").has_value());
	EXPECT_FALSE(spk::fromString("Ctrl").has_value());
	EXPECT_FALSE(spk::fromString("LeftCtrl").has_value());
	EXPECT_FALSE(spk::fromString("Semicolon").has_value());
}

TEST(KeyboardTest, InvalidTextDoesNotParse)
{
	EXPECT_FALSE(spk::fromString("").has_value());
	EXPECT_FALSE(spk::fromString("DefinitelyNotAKey").has_value());
	EXPECT_FALSE(spk::fromString(" A ").has_value());
}

TEST(KeyboardTest, InvalidKeyStringificationUsesUnknownFallback)
{
	const auto invalid = static_cast<spk::Keyboard::Key>(spk::Keyboard::NbKey + 42);
	EXPECT_EQ(spk::toString(invalid), "Unknown");
	EXPECT_EQ(spk::toWstring(invalid), L"Unknown");
}

TEST(KeyboardTest, OutOfRangeIndexThrows)
{
	// Isolate the potentially fatal MSVC debug-array assertion so an unmet
	// exception contract fails this test without terminating the whole suite.
	EXPECT_EXIT(
		{
			spk::Keyboard keyboard;
			const auto invalid = static_cast<spk::Keyboard::Key>(spk::Keyboard::NbKey);
			try
			{
				(void)keyboard[invalid];
			}
			catch (const std::out_of_range &)
			{
				std::exit(0);
			}
			catch (...)
			{
				std::exit(2);
			}
			std::exit(1);
		},
		::testing::ExitedWithCode(0), "");

	EXPECT_EXIT(
		{
			const spk::Keyboard keyboard;
			const auto invalid = static_cast<spk::Keyboard::Key>(spk::Keyboard::NbKey);
			try
			{
				(void)keyboard[invalid];
			}
			catch (const std::out_of_range &)
			{
				std::exit(0);
			}
			catch (...)
			{
				std::exit(2);
			}
			std::exit(1);
		},
		::testing::ExitedWithCode(0), "");
}
