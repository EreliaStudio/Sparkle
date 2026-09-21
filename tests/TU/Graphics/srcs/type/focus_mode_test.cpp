#include <gtest/gtest.h>

#include "type/focus_mode.hpp"

#include <sstream>
#include <string>

namespace
{
	std::string pointerToString(const spk::Widget *widget)
	{
		std::ostringstream stream;
		stream << static_cast<const void *>(widget);
		return stream.str();
	}

	std::wstring pointerToWString(const spk::Widget *widget)
	{
		std::wostringstream stream;
		stream << static_cast<const void *>(widget);
		return stream.str();
	}
}

TEST(FocusModeTest, ChannelToStringReturnsExpectedValues)
{
	EXPECT_EQ(spk::toString(spk::FocusMode::Channel::Keyboard), "Keyboard");
	EXPECT_EQ(spk::toString(spk::FocusMode::Channel::Mouse), "Mouse");
}

TEST(FocusModeTest, ChannelToWStringReturnsExpectedValues)
{
	EXPECT_EQ(spk::toWString(spk::FocusMode::Channel::Keyboard), L"Keyboard");
	EXPECT_EQ(spk::toWString(spk::FocusMode::Channel::Mouse), L"Mouse");
}

TEST(FocusModeTest, ChangeTypeToStringReturnsExpectedValues)
{
	EXPECT_EQ(spk::toString(spk::FocusMode::ChangeType::Take), "Take");
	EXPECT_EQ(spk::toString(spk::FocusMode::ChangeType::Release), "Release");
}

TEST(FocusModeTest, ChangeTypeToWStringReturnsExpectedValues)
{
	EXPECT_EQ(spk::toWString(spk::FocusMode::ChangeType::Take), L"Take");
	EXPECT_EQ(spk::toWString(spk::FocusMode::ChangeType::Release), L"Release");
}

TEST(FocusModeTest, EnumOutputStreamsReturnExpectedValues)
{
	std::ostringstream stream;
	stream << spk::FocusMode::Channel::Keyboard << ' '
		   << spk::FocusMode::Channel::Mouse << ' '
		   << spk::FocusMode::ChangeType::Take << ' '
		   << spk::FocusMode::ChangeType::Release;
	EXPECT_EQ(stream.str(), "Keyboard Mouse Take Release");

	std::wostringstream wideStream;
	wideStream << spk::FocusMode::Channel::Keyboard << L' '
			   << spk::FocusMode::Channel::Mouse << L' '
			   << spk::FocusMode::ChangeType::Take << L' '
			   << spk::FocusMode::ChangeType::Release;
	EXPECT_EQ(wideStream.str(), L"Keyboard Mouse Take Release");
}

TEST(FocusModeTest, InvalidEnumValuesProduceUnknownToken)
{
	const auto invalidChannel = static_cast<spk::FocusMode::Channel>(42);
	const auto invalidChangeType = static_cast<spk::FocusMode::ChangeType>(42);

	EXPECT_EQ(spk::toString(invalidChannel), "Unknown");
	EXPECT_EQ(spk::toWString(invalidChannel), L"Unknown");
	EXPECT_EQ(spk::toString(invalidChangeType), "Unknown");
	EXPECT_EQ(spk::toWString(invalidChangeType), L"Unknown");

	std::ostringstream stream;
	stream << invalidChannel << ' ' << invalidChangeType;
	EXPECT_EQ(stream.str(), "Unknown Unknown");

	std::wostringstream wideStream;
	wideStream << invalidChannel << L' ' << invalidChangeType;
	EXPECT_EQ(wideStream.str(), L"Unknown Unknown");
}

TEST(FocusModeTest, RecordStringConversionsReturnExpectedValues)
{
	int widgetStorage = 0;
	const auto widget = reinterpret_cast<spk::Widget *>(&widgetStorage);
	const spk::FocusMode::Record record{spk::FocusMode::ChangeType::Take, widget};

	EXPECT_EQ(spk::toString(record), "(Take, " + pointerToString(widget) + ")");
	EXPECT_EQ(spk::toWString(record), L"(Take, " + pointerToWString(widget) + L")");
}

TEST(FocusModeTest, RecordOutputStreamsReturnExpectedValues)
{
	int widgetStorage = 0;
	const auto widget = reinterpret_cast<spk::Widget *>(&widgetStorage);
	const spk::FocusMode::Record record{spk::FocusMode::ChangeType::Release, widget};

	std::ostringstream stream;
	stream << record;
	EXPECT_EQ(stream.str(), "(Release, " + pointerToString(widget) + ")");

	std::wostringstream wideStream;
	wideStream << record;
	EXPECT_EQ(wideStream.str(), L"(Release, " + pointerToWString(widget) + L")");
}

TEST(FocusModeTest, FocusModeStringConversionsReturnExpectedValues)
{
	int keyboardWidgetStorage = 0;
	int mouseWidgetStorage = 0;
	const auto keyboardWidget = reinterpret_cast<spk::Widget *>(&keyboardWidgetStorage);
	const auto mouseWidget = reinterpret_cast<spk::Widget *>(&mouseWidgetStorage);
	const spk::FocusMode mode{{keyboardWidget, mouseWidget}};

	EXPECT_EQ(
		spk::toString(mode),
		"(" + pointerToString(keyboardWidget) + ", " + pointerToString(mouseWidget) + ")");
	EXPECT_EQ(
		spk::toWString(mode),
		L"(" + pointerToWString(keyboardWidget) + L", " + pointerToWString(mouseWidget) + L")");
}

TEST(FocusModeTest, FocusModeOutputStreamsReturnExpectedValues)
{
	int keyboardWidgetStorage = 0;
	int mouseWidgetStorage = 0;
	const auto keyboardWidget = reinterpret_cast<spk::Widget *>(&keyboardWidgetStorage);
	const auto mouseWidget = reinterpret_cast<spk::Widget *>(&mouseWidgetStorage);
	const spk::FocusMode mode{{keyboardWidget, mouseWidget}};

	std::ostringstream stream;
	stream << mode;
	EXPECT_EQ(
		stream.str(),
		"(" + pointerToString(keyboardWidget) + ", " + pointerToString(mouseWidget) + ")");

	std::wostringstream wideStream;
	wideStream << mode;
	EXPECT_EQ(
		wideStream.str(),
		L"(" + pointerToWString(keyboardWidget) + L", " + pointerToWString(mouseWidget) + L")");
}
